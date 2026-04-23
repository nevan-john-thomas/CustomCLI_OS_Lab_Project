/*
Phase2-Remote CLI Shell: In this phase, you have to upgrade Phase1 with remote access
capability to your shell (implemented in the server) through Socket communication. In this
context, the client takes inputs from the user and sends requests to the server. Each input
from the client can be either a command (such as ls, ls-l, pwd, mkdir, rm –r, …) , composed
commands or a program to execute including its name. The client will then receive the
output/result from the server and print it on the screen.
*/

/*
Must be compiled using:
`gcc cli.c server.c -o server` with the main function in cli.c commented out
because functions implemented in cli.c are used here
*/

#include "cli.h"
#include <arpa/inet.h>
#include <sys/ioctl.h>

#define MAX_CONCURRENT_CONNECTIONS 3
#define SERVER_PORT 8080
#define BACKLOG 3

void send_size_and_string_to_client(int socket_connection, char *string, int bytes, bool canTransmit) {
    // Communicates with the client in a standardized form

    // Send the size first so that the client can prepare a buffer for the resulting text
    send(socket_connection, &bytes, sizeof(int), 0); 
    //sleep(1); // Wait so that the client can process the information

    send(socket_connection, string, bytes, 0); // Send the actual text afterward
    //sleep(1);
    
    send(socket_connection, &canTransmit, sizeof(bool), 0); 
    // Sends whether the client can transmit their own messages over the socket after this message

}

void read_pipe_and_client_forward(int server_output_pipe[2], int socket_connection, bool canTransmit) {
    close(server_output_pipe[WRITE_END]);
    int bytes_available;
    if (ioctl(server_output_pipe[READ_END], FIONREAD, &bytes_available) < 0) {
        // Handle error
        perror("Error reading from the server output pipe:\n");
        pthread_exit(NULL);
    }

    char result_string[bytes_available+1]; //+1 to account for the null terminator
    read(server_output_pipe[READ_END], result_string, sizeof(char) * bytes_available);
    result_string[bytes_available] = '\0'; // Null terminate the string
    send_size_and_string_to_client(socket_connection, result_string, bytes_available + 1, canTransmit);
    close(server_output_pipe[READ_END]);
}

void *run_cli(void *arg) {
    // The CLI needs to copy all the final STDOUT outputs to the server_pipe_fd.
    // The parent process then needs to read from the server_pipe_fd and write the results
    // through the socket to the client. (To talk to the client, the server must first
    // send the no. of character bytes it will transmit followed by the actual content)
    // the server_write_pipe_fd must only be written to in the child processes

    int socket_connection = *(int*) arg;
    free(arg); 
    // Convert the heap variable back to a stack variable and free the memory allocated on the heap

    int server_output_pipe[2];

    char buffer_string[INPUT_BUFFER_SIZE];

    static char *pwd_argv[] = {"pwd", NULL};
    static char *cat_argv[] = {"cat", "welcome.txt", NULL};
    
    Command pwd_cmd;
    pwd_cmd.argc = 1;
    pwd_cmd.argv = pwd_argv;

    Command cat_cmd;
    cat_cmd.argc = 1;
    cat_cmd.argv = cat_argv;

    pipe(server_output_pipe);
    execute_single_command(cat_cmd, server_output_pipe);
    read_pipe_and_client_forward(server_output_pipe, socket_connection, false);

    pipe(server_output_pipe); // Create a new pipe so that the next command 
    execute_single_command(pwd_cmd, server_output_pipe);
    read_pipe_and_client_forward(server_output_pipe, socket_connection, false);

    send_size_and_string_to_client(socket_connection, "\n", sizeof(char) * 2, false);

    while (true) {
        char prompt_text[] = "> ";
        send_size_and_string_to_client(socket_connection, prompt_text, sizeof(char) * (strlen(prompt_text) + 1), true);
        // printf("> ");

        read(socket_connection, buffer_string, INPUT_BUFFER_SIZE);
        // fgets(buffer_string, INPUT_BUFFER_SIZE, stdin);

        // Check for the exit command first so that the server can exit before doing any other processing
        // Ensures that server resources aren't wasted on processing the exit command, 
        // client receives a response and the server doesnt close after session termination

        buffer_string[strcspn(buffer_string, "\n")] = '\0';

        if (strcmp(buffer_string, "exit") == 0) {
             char exit_msg[] = "Exiting..\n";
             send_size_and_string_to_client(socket_connection, exit_msg, sizeof(char) * (strlen(exit_msg) + 1), false);
             close(socket_connection); 
             pthread_exit(NULL);
             return NULL;                // Return to main() — server stays running
    }
        CommandInfo cmdInfo = ParseAllCommands(buffer_string);

        if (cmdInfo.command_count != 0) {
            pipe(server_output_pipe);

            ExecuteCommands(cmdInfo, server_output_pipe);
            read_pipe_and_client_forward(server_output_pipe, socket_connection, false);

            free_commands_array_memory(cmdInfo);
        } else {
            char error_txt[] = "No command was provided!\n";
            send_size_and_string_to_client(socket_connection, error_txt, sizeof(char) * (strlen(error_txt) + 1), false);
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    int server_socket;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket object created!\n");
    
    if (server_socket < 0) {
        perror("Socket Creation Failed:\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    // Server Address options
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind Server Address to the Socket
    if (bind(server_socket, (const struct sockaddr*) &server_addr, addrlen) < 0) {
        perror("Socket Bind Failed:\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Socket bind succeeded!\n");
    
    // Listen for connections
    printf("Beginning to listen for socket connections.\n");
    if (listen(server_socket, BACKLOG) < 0) {
        perror("Listening for Socket Connections Failed:\n");
        exit(EXIT_FAILURE);
    }

    int* socket_connection_val_ptr;
    int socket_connection;
    
    // Keep accepting new clients
    while (true) {
        printf("Waiting to accept a client..\n");
        socket_connection = accept(server_socket, (struct sockaddr*) &server_addr, &addrlen);
        
        if (socket_connection < 0) {
            perror("Failed to accept the client!\n");
            close(server_socket);
            continue;
        }

        printf("Client successfully connected! Running CLI....\n");
        
        pthread_t thread;
        pthread_attr_t attribute;

        socket_connection_val_ptr = (int*) malloc(sizeof(int)); 
        // Allocate memory on the heap so that the socket connection value 
        // can be accessed within the thread until the end of its execution

        *socket_connection_val_ptr = socket_connection;

        pthread_attr_init(&attribute);
        pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread, &attribute, run_cli, socket_connection_val_ptr);

    }

    /*
    Once a client connection has been accepted. A while loop needs to be established
    until the exit command is provided, in which case, the connection should be terminated.

    While the client is connected, any STDOUT outputs from the server should be forwarded to the client
    while keeping multithreading in mind.
    */

    /*
    For sending from the client to the server, keep the fixed input buffer of 1000 bytes
    
    For sending from the server back to the client, since the response length isn't fixed, 
    everytime some new text comes in through STDOUT, we send the length of that to the 
    client first and then the client creates a string buffer with that size and 
    then receives the actual value

    All of this should keep threads in mind though

    Perhaps each client that connects can be identified by the IP Address and Port Number, 
    or maybe some other ID? The accept function returns a number which might be sufficient
    */

    return 0;
}