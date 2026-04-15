/*
Phase2-Remote CLI Shell: In this phase, you have to upgrade Phase1 with remote access
capability to your shell (implemented in the server) through Socket communication. In this
context, the client takes inputs from the user and sends requests to the server. Each input
from the client can be either a command (such as ls, ls-l, pwd, mkdir, rm –r, …) , composed
commands or a program to execute including its name. The client will then receive the
output/result from the server and print it on the screen.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define BACKLOG 3

int main(int argc, char **argv) {
    int server_socket;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0); 

    if (server_socket == 0) {
        perror("Socket Creation Failed:\n");
        exit(EXIT_FAILURE);
    }

    // Server Address options
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind Server Address to the Socket
    if (bind(server_socket, (const struct sockaddr*) &server_addr, addrlen) < 0) {
        perror("Socket Bind Failed:\n");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, BACKLOG) < 0) {
        perror("Listening for Socket Connections Failed:\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}