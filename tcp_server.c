#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <math.h>      // for fabs()
#include "utils.h"

#define numExternals 4
#define EPSILON 1.  // threshold for "near equal" stability

// ------------------------------------------------------------
// Function to establish connections with external processes
// ------------------------------------------------------------
int * establishConnectionsFromExternalProcesses()
{
    int socket_desc;
    static int client_socket[numExternals];
    unsigned int client_size;
    struct sockaddr_in server_addr, client_addr;

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        exit(0);
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind socket:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        printf("Couldn't bind to the port\n");
        exit(0);
    }
    printf("Done with binding\n");

    // Listen:
    if(listen(socket_desc, 1) < 0){
        printf("Error while listening\n");
        exit(0);
    }

    printf("\n\nListening for incoming connections.....\n\n");
    printf("-------------------- Initial connections ---------------------------------\n");

    int externalCount = 0;
    while (externalCount < numExternals){
        client_socket[externalCount] = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
        if (client_socket[externalCount] < 0){
            printf("Can't accept\n");
            exit(0);
        }

        printf("One external process connected at IP: %s and port: %i\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        externalCount++;
    }

    printf("--------------------------------------------------------------------------\n");
    printf("All four external processes are now connected\n");
    printf("--------------------------------------------------------------------------\n\n");

    // Close the listening socket cause we got all four
    close(socket_desc);

    return client_socket;
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main(void)
{
    struct msg messageFromClient;
    int * client_socket = establishConnectionsFromExternalProcesses();

    float temperature[numExternals];
    float centralTemp = 0.0;
    bool stable = false;

    while (!stable) {
        printf("--------------------------------------------------------------\n");

        // Receive temperatures from all external processes
        for (int i = 0; i < numExternals; i++) {
            if (recv(client_socket[i], (void *)&messageFromClient, sizeof(messageFromClient), 0) < 0) {
                printf("Couldn't receive from client %d\n", i);
                return -1;
            }
            temperature[i] = messageFromClient.T;
            printf("Received from External (%d): %.4f\n", i, temperature[i]);
        }

        //adds all temps together
        float sumExternals = 0.0;
        for (int i = 0; i < numExternals; i++) {
            sumExternals += temperature[i];
        }

      // formula
        float newCentralTemp = (2 * centralTemp + sumExternals) / 6.0;

        printf("central temp = %.4f\n",  newCentralTemp);
        centralTemp = newCentralTemp;

        // Construct message to send back to clients
        struct msg updated_msg;
        updated_msg.T = centralTemp;
        updated_msg.Index = 0;

        // Check if system is stable
        stable = true;
        for (int i = 0; i < numExternals; i++) {
            if (fabs(temperature[i] - centralTemp) > EPSILON) {
                stable = false;
                break;
            }
        }

        //is it stable or nah and if so send that to the clients so they can close
        //unsure if this is nesscairy cause realisticly i could have misentrapted the instructions
        // but it doesent really affect anything and it works regardless
        if (stable)
            strcpy(updated_msg.status, "stable");
        else
            strcpy(updated_msg.status, "unstable");

        // we send the new temp to the clients so they can update their external temps
        for (int i = 0; i < numExternals; i++) {
            if (send(client_socket[i], (const void *)&updated_msg, sizeof(updated_msg), 0) < 0) {
                printf("Can't send to client %d\n", i);
                return -1;
            }
        }

    }

    printf("==============================================================\n");
    printf("System reached stabilit\n");
    printf("central temperature = %.4f\n", centralTemp);
    printf("==============================================================\n");

    // Close all sockets
    for (int i = 0; i < numExternals; i++)
        close(client_socket[i]);

    return 0;
}
