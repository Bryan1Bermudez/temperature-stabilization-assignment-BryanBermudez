#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"

int main (int argc, char *argv[])
{
    int socket_desc;
    struct sockaddr_in server_addr;
    struct msg the_message; 
    
    if (argc < 3) {
        printf("Usage: %s <externalIndex> <initialTemperature>\n", argv[0]);
        return -1;
    }

    // Command-line arguments
    int externalIndex = atoi(argv[1]); 
    float externalTemp = atof(argv[2]);   // initial external temperature
    
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    printf("Socket created successfully (External %d)\n", externalIndex);
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to central process
    if (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect to server\n");
        return -1;
    }
    printf("External %d connected with server successfully\n", externalIndex);
    printf("--------------------------------------------------------\n\n");

    // Prepare and send the initial message
    the_message = prepare_message(externalIndex, externalTemp);

    while (1) {
        // Send external temperature to server
        if (send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0) {
            printf("Unable to send message\n");
            break;
        }

        // Receive central temperature and status from server
        if (recv(socket_desc, (void *)&the_message, sizeof(the_message), 0) < 0) {
            printf("Error while receiving server's message\n");
            break;
        }

        // If stable stop loop and close
        // not sure if i need to do it like this but it works
        if (strcmp(the_message.status, "stable") == 0) {
            printf("--------------------------------------------------------\n");
            printf("External %d: System reached stability.\n", externalIndex);
            printf("Final central temperature = %f\n", the_message.T);
            break;
        }

        // update temperature using formula
        float centralTemp = the_message.T;
        externalTemp = (3 * externalTemp + 2 * centralTemp) / 5.0;


        // Prepare next message
        the_message = prepare_message(externalIndex, externalTemp);
    }

    // Close the socket
    close(socket_desc);
    printf("External %d shutting down.\n", externalIndex);
    return 0;
}
