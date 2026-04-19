/* 
NS CLI (Phase 2)
by Nevan John Thomas - 100064872 & Salaheddine Metnani - 100064666
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include "cli.h"


void display_commands(CommandInfo cmdInfo) {
    /*
    Used for debugging purposes to display the parsed commands
    and their arguments.
    */

    int cmd_count = cmdInfo.command_count;
    Command *commands = cmdInfo.cmds;

    printf("Commands:\n");
    for (int i = 0; i < cmd_count; i++) { // Loops through the list of commands
        Command cmd = commands[i];
        int argc = cmd.argc;

        printf("{"); // Prints each command on a newline with ` backticks for each argument separated by commas
        for (int j = 0; j < argc; j++) {
            printf("`%s`", cmd.argv[j]);
            if (j != argc - 1) printf(", ");
        }
        printf("}\n");
    }
    printf("===\n");
}

char* slice_string(char *source, int start_index, int end_index) {
    /*
    Creates a new substring from a provided source string and terminates the
    created string properly with a null so that it is valid. (ie. creates a string slice)
    the new substring includes the start but excludes the character at the end index
    */

    int remaining_str_len = strlen(source + start_index);
    int substr_length = end_index - start_index;

    if (substr_length > remaining_str_len) {
        end_index = remaining_str_len;
        substr_length = remaining_str_len;
    }


    char *dest = (char *) malloc((sizeof (char) * (substr_length + 1))); 
    // Allocating memory for the new substring.
    
    strncpy(dest, source + start_index, substr_length); // dest, source, num bytes
    dest[substr_length] = '\0';

    return dest;
}

Command ParseStringToCommand(char *string_ptr, int length) {
    /*
    Parses the provided string upto a certain length 
    into a Command object with the number of arguments
    and a respective null terminated array with strings
    representing each argument
    */

    // Calculates the argument count first to allocate the appropriate
    // amount of memory later
    int argc = 0;

    bool encounteredQuote = false;
    bool encounteredSlash = false;
    bool encounteredChar = false;

    // Loops through the string upto the desired length one time
    // and determines the number of arguments by incrementing
    // argc everytime a non-whitespace character is encountered after a series of whitespace characters 
    // (or at the start) of the string.
    // It accounts for quotation marks (assuming both open types of quotes are the same) and backslash for passing in spaces
    // It does this by ignoring whitespaces if a quotation mark was seen and another quotation mark wasn't
    // 
    for (int i = 0; i < length; i++) {
        char c = string_ptr[i];

        if (encounteredChar && isspace(c) && !encounteredQuote) { // If a whitespace is encountered at the end of an argument
            if (encounteredSlash) {
                encounteredSlash = false;
                continue;
            }
            
            // Reached the end of an argument
            encounteredChar = false;
            continue;
        };

        if (isspace(c)) continue; // Ignore all other whitespace

        // If the code got to this point then the character must not be a whitespace
        if (!encounteredChar) argc++; // Increment argc if this is the first character after a series of whitespace characters
        encounteredChar = true;

        if (c == '\"' || c == '\'') encounteredQuote = !encounteredQuote; 
        // Allows for someone to mix quotes in arguments

        // encounteredSlash must be false always except while processing a single character 
        // immediately after the \ character was seen in the string

        if (c == '\\' && !encounteredSlash) { 
            encounteredSlash = true;
        } else {
            encounteredSlash = false;
        }
    }

    // Defines the command and allocates memory for the arguments
    // printf("Argc: %d\n", argc);
    Command cmd;
    cmd.argc = argc;
    
    if (argc == 0) {
        cmd.argv = NULL;
        return cmd;
    }

    // Allocate memory on the Heap for the arguments
    char **argv = (char **) malloc((sizeof (char *)) * (argc + 1));
    argv[argc] = NULL; // Null terminate the array of arguments so that it can directly be passed for the execvp function

    cmd.argv = argv;

    // Looping through the string again to extract each argument this time
    int arg_index = 0;
    encounteredQuote = false;
    encounteredSlash = false;
    encounteredChar = false;

    // Utilizes the same logic as before (except at the last character incase a quotation mark wasn't closed)
    // but maintains a record of the start and end of the argument within
    // the string to create a slice and store it into the argv array 
    int startIndex = 0;
    int endIndex = 0;
    int substr_length = 0;

    for (int i = 0; i < length; i++) {
        char c = string_ptr[i];

        if (encounteredChar && isspace(c) && !encounteredQuote) { // If a whitespace is encountered at the end of an argument
            if (encounteredSlash && (i != length - 1)) { // The != length - 1 is to ensure that argument is captured properly if it ends with a backslash
                encounteredSlash = false;
                continue;
            }
            
            // Reached the end of an argument
            encounteredChar = false;
            
            // Copy the argument into the string
            endIndex = i;

            char prev_c = string_ptr[endIndex - 1];
            if (prev_c == '\'' || prev_c == '\"') { 
                // This is used to account for the fact that the whitespace might follow
                // after a quotation mark, thereby making the endIndex include it.
                // This will not raise a segfault because the character at the 0th index can
                //  never enter this encounteredChar block 
                endIndex = i - 1;
            }

            argv[arg_index] = slice_string(string_ptr, startIndex, endIndex);
            arg_index++;
            continue;
        };

        if (isspace(c)) continue; // Ignore all other whitespace

        // If the code got to this point then the character must not be a whitespace
        if (!encounteredChar) {
            startIndex = i;
        }; // Increment argc if this is the first character after a series of whitespace characters
        encounteredChar = true;

        if (encounteredChar && (i == length - 1)) { // ie. it's at the last character 
            // To account for the case where double quotes weren't properly closed before this point
            
            if (c != '\"' && c != '\'') {
                endIndex = i + 1; // +1 so that the last character is also included
            } else {
                endIndex = i; // so that the closing parenthesis isn't included
            }
            argv[arg_index] = slice_string(string_ptr, startIndex, endIndex);
            arg_index++;
        }

        if ((c == '\"' || c == '\'') && encounteredQuote && !encounteredSlash) {
            encounteredQuote = false; // So that the closing parenthesis isn't included
            endIndex = i - 1;
        } else if ((c == '\"' || c == '\'') && !encounteredQuote && !encounteredSlash) {
            encounteredQuote = true;
            startIndex = i + 1; // So that the opening parenthesis isn't included
        }
        // Allows for someone to mix quotes in arguments

        if (c == '\\' && !encounteredSlash) { 
            encounteredSlash = true;
        } else {
            encounteredSlash = false;
        }
    }

    return cmd;
}

CommandInfo ParseAllCommands(char *string) {
    int pipe_count = 0;

    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == '|') pipe_count += 1;
    }

    int cmd_count = pipe_count + 1;
    
    Command *cmds = (Command *) malloc(sizeof(Command) * cmd_count);
    int cmd_index = 0;
    
    char *cmd_ptr = string;
    int cmd_len;
    while (true) {
        cmd_len = strcspn(cmd_ptr, "|");
        // printf("Cmd Len: %d, Str Len: %lu, Parsing Command: %s\n", cmd_len, strlen(cmd_ptr), slice_string(cmd_ptr, 0, cmd_len));
        cmds[cmd_index] = ParseStringToCommand(cmd_ptr, cmd_len);

        cmd_ptr += cmd_len;
        if (*cmd_ptr == '\0') {
            break;
        } if (*cmd_ptr == '|') {
            cmd_ptr++;
        }

        cmd_index++;
    }
    
    CommandInfo cmd_info;
    cmd_info.command_count = cmd_count;
    cmd_info.cmds = cmds;

    // Final loops to remove empty commands (ie. commands with 0 args)
    // and to reallocate memory for the remaining commands

    int trimmed_cmd_count = cmd_count;
    for (int i = 0; i < cmd_count; i++) {
        Command cmd = cmds[i];
        if (cmd.argc == 0) trimmed_cmd_count--;
    }
    
    if (trimmed_cmd_count == cmd_count) return cmd_info; 
    // No empty commands were found, so do not reallocate memory and return as usual
    
    Command *trimmed_cmds = (Command *) malloc(sizeof(Command) * trimmed_cmd_count); // Creates a new block of memory
    
    int j = 0;
    for (int i = 0; i < cmd_info.command_count; i++) {
        Command cmd = cmds[i];
        if (cmd.argc != 0) {
            trimmed_cmds[j] = cmds[i];
            j++;
        };
    }

    free(cmds);
    cmd_info.command_count = trimmed_cmd_count;
    cmd_info.cmds = trimmed_cmds;

    return cmd_info;
}

void execute_commands_and_direct_output(Command cmd_left, Command cmd_right, int fd_parent[2]) {
    /*
    This must be a child process because the piped output must also be executed
    and that would kill the program if this was not a fork
    fd_parent represents the file descriptor of the parent to direct 
    the output to if required (fd_parent can be NULL).
    */

    int fd_child[2];
    if (pipe(fd_child) == -1) {
        printf("Failed to create a pipe!\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pid = fork();

    int status_code;

    if (pid < 0) {
        printf("Process creation failed!\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        close(fd_child[READ_END]);
        dup2(fd_child[WRITE_END], STDOUT_FILENO);
        // execlp("cat", "cat", "file.txt", NULL);

        char **argv = cmd_left.argv;
        char *file = argv[0];

        execvp(file, argv);

        perror("Command execution failed!\n");

        close(fd_child[WRITE_END]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(&status_code);
        close(fd_child[WRITE_END]);
        dup2(fd_child[READ_END], STDIN_FILENO);
        
        if (fd_parent != NULL) {
            close(fd_parent[READ_END]);
            dup2(fd_parent[WRITE_END], STDOUT_FILENO);
        }

        char **argv = cmd_right.argv;
        char *file = argv[0];

        execvp(file, argv);

        perror("Command execution failed!\n");

        if (fd_parent != NULL) {
            close(fd_parent[WRITE_END]);
        }

        close(fd_child[READ_END]);
        exit(EXIT_FAILURE);
    }
}

void recursive_execute(Command *cmds, int depth, int command_count, int fd_parent[2]) {
    // printf("Entering -> Depth: %d, PID: %d\n", depth, getpid());

    if (depth < (command_count - 2)) { // Recursively run this until only two commands remain
        int fd_child[2];
        if (pipe(fd_child) == -1) {
            printf("Failed to create a pipe!\n");
            exit(EXIT_FAILURE);
        }

        Command cmd_right = cmds[command_count - depth - 1];

        pid_t pid;
        pid = fork();

        int status_code;

        if (pid < 0) {
            perror("Process Creation Failed!\n");
            return;
        }

        if (pid == 0) { // Child Process
            // printf("Child -> Depth: %d, PID: %d\n", depth, getpid());
            recursive_execute(cmds, depth + 1, command_count, fd_child);

        } else { // Parent Process
            // printf("Waiting for Child -> Depth: %d, PID: %d\n", depth, pid);
            wait(&status_code); // Wait for the child to finish executing
            
            close(fd_child[WRITE_END]);
            dup2(fd_child[READ_END], STDIN_FILENO);

            if (fd_parent != NULL) {
                close(fd_parent[READ_END]);
                dup2(fd_parent[WRITE_END], STDOUT_FILENO);
            }

            char **argv = cmd_right.argv;
            char *file = argv[0];

            execvp(file, argv);

            perror("Command execution failed!\n");
            
            if (fd_parent != NULL) close(fd_parent[WRITE_END]);

            close(fd_child[READ_END]);
            exit(EXIT_FAILURE);

            if (status_code == EXIT_FAILURE) {
                perror("Error:\n");
                printf("Command execution failed in the Child Process!\n");
                return;
            }
        }
    } else {
        // printf("Base Case -> Depth: %d, PID: %d\n", depth, getpid());

        Command cmd_left = cmds[0];
        Command cmd_right = cmds[1];
        
        execute_commands_and_direct_output(
            cmd_left, cmd_right, fd_parent
        );
    }
}

void execute_single_command(Command cmd, int fd_parent[2]) {
    pid_t pid;
    pid = fork();

    int status_code;

    if (pid < 0) {
        perror("Process Creation Failed!\n");
        return;
    }

    if (pid == 0) { // Child Process
        // printf("In Child Process:\n");
        char **argv = cmd.argv;
        char *file = argv[0];

        if (fd_parent != NULL) {
            close(fd_parent[READ_END]);
            dup2(fd_parent[WRITE_END], STDOUT_FILENO);
        }

        execvp(file, argv);

        // If this code runs that means execution failed
        if (fd_parent != NULL) close(fd_parent[WRITE_END]);
        exit(EXIT_FAILURE);
    } else { // Parent Process
        wait(&status_code); // Wait for the child to finish executing
        if (status_code == EXIT_FAILURE) {
            perror("Error:\n");
            printf("Command execution failed in the Child Process!\n");
            return;
        }
    }
}

void ExecuteCommands(CommandInfo cmd_info, int server_pipe_fd[2]) {
    int cmd_count = cmd_info.command_count;
    Command *cmds = cmd_info.cmds;

    // Simple command execution when chaining results through a pipe isn't required
    // Handles certain built-in commands as well
    if (cmd_count == 1) {
        Command cmd = cmds[0];
        if (cmd.argc == 0) return;
        // if (strcmp(cmd.argv[0], "exit") == 0) {
        //     printf("Exiting..\n");
        //     exit(EXIT_SUCCESS);
        //}
        if (strcmp(cmd.argv[0], "cd") == 0) {
            if (cmd.argc != 2) {
                printf("`cd` takes in exactly one argument!\n");
                return;
            }
            chdir(cmd.argv[1]);
            return;
        }

        execute_single_command(cmds[0], server_pipe_fd);
        return;
    } 

    pid_t pid;
    pid = fork();

    int status_code;

    if (pid < 0) {
        perror("Process Creation Failed!\n");
        return;
    }

    if (pid == 0) { // Child Process
        if (cmd_count == 2) {
            execute_commands_and_direct_output(cmds[0], cmds[1], server_pipe_fd);
            return;
        } else {
            recursive_execute(cmds, 0, cmd_count, server_pipe_fd);
        }

        // If this code runs that means execution failed
        exit(EXIT_FAILURE);
    } else { // Parent Process
        wait(&status_code); // Wait for the child to finish executing 

        if (status_code == EXIT_FAILURE) {
            perror("Error:\n");
            printf("Command execution failed in the Child Process!\n");
            return;
        }
    }

}

void free_command_memory(Command cmd) {
    // Deallocates the memory associated with the argument value strings
    // that were copied and stored in the argv array after parsing the command, 
    // and the memory allocated for the argument argv array of pointers itself.

    for (int i = 0; i < cmd.argc; i++) {
        free(cmd.argv[i]); // Free memory for each argv string
        cmd.argv[i] = NULL;
    }

    free(cmd.argv); // Free memory for argv string pointers
    cmd.argc = 0;
    cmd.argv = NULL;
}

void free_commands_array_memory(CommandInfo cmd_info) {
    // Frees memory for an array of commands
    // and frees any memory acquired for the creation of each
    // individual command as well.

    for (int i = 0; i < cmd_info.command_count; i++) {
        free_command_memory(cmd_info.cmds[i]);
    }

    free(cmd_info.cmds);
    cmd_info.command_count = 0;
    cmd_info.cmds = NULL;
}