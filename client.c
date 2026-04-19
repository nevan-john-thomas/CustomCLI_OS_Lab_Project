/* 
NS CLI (Phase 2)
by Nevan John Thomas - 100064872 & Salaheddine Metnani - 100064666
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define INPUT_BUFFER_SIZE 1000 // This value must be the same as the value in cli.h
#define SERVER_PORT 8080
#define BACKLOG 3

bool display_server_output_and_transmit_permission(int socket_connection) {
    int bytes_to_receive;
    // read() returns 0 when the server closes the connection.
    // Without this check the client loops forever printing garbage.
    int bytes_read = read(socket_connection, &bytes_to_receive, sizeof(int));
    if (bytes_read <= 0) {
        printf("\nServer closed the connection.\n");
        close(socket_connection);
        exit(EXIT_SUCCESS); // Exit the client cleanly
    }

    char server_output[bytes_to_receive];
    read(socket_connection, server_output, sizeof(char) * bytes_to_receive);
    printf("%s", server_output);

    bool canTransmit;
    read(socket_connection, &canTransmit, sizeof(bool));
    return canTransmit;
}

int main(int argc, char **argv) {
    int client_socket;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    client_socket = socket(AF_INET, SOCK_STREAM, 0); 
    printf("Socket object created!\n");
    
    if (client_socket < 0) {
        perror("Socket Creation Failed:\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    
    // Server Address options
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) < 0) {
        printf("Invalid address or address is not supported!\n");
        exit(EXIT_FAILURE);
    }
    printf("Socket address assigned!\n");
    
    printf("Attempting to connect to the server!\n");
    if (connect(client_socket, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("Connection to the server failed!\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connection to the server has been established!\n");

    bool canTransmit;
    char buffer_string[INPUT_BUFFER_SIZE];

    while (true) {
        // Continuously listen for data from the server and wait for the canTransmit flag to be true
        // to transmit data back to the server.
        canTransmit = display_server_output_and_transmit_permission(client_socket);
        
        if (canTransmit) {
            fgets(buffer_string, INPUT_BUFFER_SIZE, stdin);
            send(client_socket, buffer_string, INPUT_BUFFER_SIZE, 0);
            memset(buffer_string, 0, INPUT_BUFFER_SIZE); // Clear the input buffer after sending the text
        }
    }

    return 0;
}