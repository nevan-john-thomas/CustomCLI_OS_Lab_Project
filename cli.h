#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/wait.h>
#include <pthread.h>

#define READ_END 0
#define WRITE_END 1
#define INPUT_BUFFER_SIZE 1000

typedef struct Command
{
    int argc;
    char **argv; // Must be NULL pointer terminated regardless so it can be directly passed to the execvp function
} Command; 
// Represents a single command passed into the custom shell

typedef struct CommandInfo
{
    int command_count;
    Command *cmds; // Must be NULL pointer terminated regardless so it can be directly passed to the execvp function
} CommandInfo; 

void display_commands(CommandInfo cmdInfo);

char* slice_string(char *source, int start_index, int end_index);

Command ParseStringToCommand(char *string_ptr, int length);

CommandInfo ParseAllCommands(char *string);

void execute_commands_and_direct_output(Command cmd_left, Command cmd_right, int fd_parent[2]);

void recursive_execute(Command *cmds, int depth, int command_count, int fd_parent[2]);

void execute_single_command(Command cmd, int fd_parent[2]);

void ExecuteCommands(CommandInfo cmd_info, int server_pipe_fd[2]);

void free_command_memory(Command cmd);

void free_commands_array_memory(CommandInfo cmd_info);