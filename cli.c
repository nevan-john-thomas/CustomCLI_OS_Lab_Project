/* 
Phase1-Local CLI Shell: In this phase, you have to develop using C your own shell/CLI that
replicates features from the linux one such as ls, ls-l, pwd, mkdir, rm, | , … or a program to
execute including its name. You should implement at least 15 commands and should include
the composed commands including 1 pipe (…|…), 2 pipes (…|…|…) and 3 pipes (…|…|…|…).
You can use the techniques that you have learned about creating processes (e.g. fork, excec),
interprocess communications (e.g. pipes), etc.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct Command
{
    int argc;
    char **argv;
} Command; 
// Represents a single command passed into the custom shell

void displayCommands(Command commands[], int cmd_count) {
    /*
    Used for debugging purposes to display the parsed commands
    and their arguments
    */

    printf("Commands:\n");
    for (int i = 0; i < cmd_count; i++) {
        Command cmd = commands[i];
        int argc = cmd.argc;
        printf("{");
        for (int j = 0; j < argc; j++) {
            printf("`%s`", cmd.argv[j]);
            if (j != argc - 1) printf(", ");
        }
        printf("}\n");
    }
    printf("===\n");
}

char *copySubstring(char *source, int start_index, int end_index) {
    /*
    Copies a substring from one string to another and terminates the
    destination string properly with a null so that it is a valid string.
    the copied string includes the start but excludes the character at the end index
    */
    int substr_length = end_index - start_index;

    char *dest = (char *) malloc((sizeof (char) * (substr_length + 1))); 
    // Allocating memory for the string that's supposed to be here.
    
    strncpy(dest, source, substr_length); // dest, source, num bytes
    dest[substr_length] = '\0';

    return dest;
}

Command parseStringToCommand(char *string_ptr, int length) {
    int argc = 0;

    /* 
    Counts the numbers of arguments in the string
    */

    bool encounteredQuote = false;
    bool encounteredSlash = false;
    bool encounteredChar = false;

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

        if (c == '\\') encounteredSlash = true;
    }

    // Defines the command and allocates memory for the argument
    Command cmd;
    cmd.argc = argc;
    
    if (argc == 0) {
        cmd.argv = NULL;
        return cmd;
    }

    // Allocate memory on the Heap for the arguments
    char **argv = (char **) malloc((sizeof (char *)) * (argc + 1));
    argv[argc] = NULL; // Null terminate the array of arguments

    cmd.argv = argv;

    // Looping through the string again to extract each argument this time
    int arg_index = 0;
    encounteredQuote = false;
    encounteredSlash = false;
    encounteredChar = false;

    int startIndex = 0;
    int endIndex = 0;
    int substr_length = 0;

    for (int i = 0; i < length; i++) {
        char c = string_ptr[i];

        if (encounteredChar && (i == length - 1)) { // ie. it's at the last character 
            // To account for the case where double quotes weren't properly closed
            // or if it ends right after a backslash character
            endIndex = i + 1; // +1 so that the last character is also included 
            argv[arg_index] = copySubstring(string_ptr + startIndex, startIndex, endIndex);
            arg_index++;
        } 

        if (encounteredChar && isspace(c) && !encounteredQuote) { // If a whitespace is encountered at the end of an argument
            if (encounteredSlash) {
                encounteredSlash = false;
                continue;
            }
            
            // Reached the end of an argument
            encounteredChar = false;
            
            // Copy the argument into the string
            endIndex = i;
            argv[arg_index] = copySubstring(string_ptr + startIndex, startIndex, endIndex);
            arg_index++;
            continue;
        };

        if (isspace(c)) continue; // Ignore all other whitespace

        // If the code got to this point then the character must not be a whitespace
        if (!encounteredChar) {
            startIndex = i;
        }; // Increment argc if this is the first character after a series of whitespace characters
        encounteredChar = true;

        if (c == '\"' || c == '\'') encounteredQuote = !encounteredQuote; 
        // Allows for someone to mix quotes in arguments

        if (c == '\\') encounteredSlash = true;
    }

    return cmd;
}

int main(int argc, char **argv) {
    // 1) Take in commands from the command-line (have a list of possible commands)

    // Examples:
    // ls -al
    // cat paragraph.txt | grep "word"
    // pwd

    // char *string = "ls -al | grep 'word'";
    // char *string = "        ";
    char *string = " ls -al \'one arg\' h\\ ey";
    // char *string = "ls -al";

    // char *substring;
    // substring = copySubstring(string + 3, 3, 5);

    // printf("%s\n", substring);


    // printf("%d\n", (int) strcspn(string, "|"));
    Command cmd = parseStringToCommand(string, strlen(string));
    Command cmds[] = {cmd};

    displayCommands(cmds, 1);

    // int length = strlen(string);

    // int pipe_count = 0;

    // for (char *char_ptr = string; *char_ptr != '\0' || *char_ptr != "\n"; char_ptr++) {
    //     if (*char_ptr == '|') pipe_count += 1;
    // }

    // int command_count = pipe_count + 1;
    // char (**commands)[command_count];
    // int command_index = 0;

    
    // char *char_ptr = string; // Start the character pointer at the start of the command string
    // for (int i = 0; i < command_count; i++) {

    //     int whitespace_count = 0;

    //     // Counts the number of whitespace characters in the command before breaking it apart
    //     for (char *command_ptr = char_ptr; 
    //         (*command_ptr != '|' || *command_ptr != '\0'); 
    //         command_ptr++) {
    //         if (isWhitespace(command_ptr)) pipe_count += 1;
    //     }

    //     char (*command)[whitespace_count];
    // }

// Function	Description
// memchr()	Returns a pointer to the first occurrence of a value in a block of memory
// memcmp()	Compares two blocks of memory to determine which one represents a larger numeric value
// memcpy()	Copies data from one block of memory to another
// memmove()	Copies data from one block of memory to another accounting for the possibility that the blocks of memory overlap
// memset()	Sets all of the bytes in a block of memory to the same value
// strcat()	Appends one string to the end of another
// strchr()	Returns a pointer to the first occurrence of a character in a string
// strcmp()	Compares the ASCII values of characters in two strings to determine which string has a higher value
// strcoll()	Compares the locale-based values of characters in two strings to determine which string has a higher value
// strcpy()	Copies the characters of a string into the memory of another string
// strcspn() 	Returns the length of a string up to the first occurrence of one of the specified characters
// strlen() Return the length of a string

    // 2) Parse the string, create an array of arrays of strings
    // [The outer array logically represents the commands to be executed and piped together, 
    // whereas the inner array represents the command itself with its respective parameters (as an array of strings)] 

    // printf("Args: %d\n", argc);
    // for (int i = 0; i < argc; i++) {
    //     printf("%s ", argv[i]);
    // }
    // printf("\n");

    // 3) Keep track of the number of pipe operators
    // 4) Check if the commands are valid (ie. within the list of 15 accepted commands)
    // Keep the parent process strictly for input -> fork within the child process for each pipe operator 
    // The topmost child process should resolve the (pipe-count - 1)'th command and the first command should be resolved by the pipe-count'th child
    // Create a function to run a command with given arguments -> fork the current process and run execv() on the child

    return 0;
}