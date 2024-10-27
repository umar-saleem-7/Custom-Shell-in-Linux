#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 1024 // Maximum length of command line
#define MAX_ARGS 64   // Maximum number of arguments

// Signal handler to reap zombie processes for background processes
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to read a line of input from the user
void read_cmd(char *line) {
    printf("myshell:- ");
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        perror("Error reading input");
        exit(EXIT_FAILURE);
    }
}

// Function to split the input line into arguments
int tokenize(char *line, char **args) {
    int i = 0;
    args[i] = strtok(line, " \t\n"); // Split by spaces and newlines
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
    return i;
}

// Execute a command with redirection and piping
void execute(char **args, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: Execute the command
        int in_fd = -1, out_fd = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "<") == 0) {
                in_fd = open(args[i+1], O_RDONLY);
                if (in_fd < 0) {
                    perror("Failed to open input file");
                    exit(EXIT_FAILURE);
                }
                dup2(in_fd, STDIN_FILENO);
                args[i] = NULL;
            } else if (strcmp(args[i], ">") == 0) {
                out_fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd < 0) {
                    perror("Failed to open output file");
                    exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                args[i] = NULL;
            }
        }
        
        if (execvp(args[0], args) < 0) {
            perror("Execution failed");
            exit(EXIT_FAILURE);
        }
    } else {
        if (background) {
            printf("[1] %d\n", pid); // Display background process PID
        } else {
            int status;
            waitpid(pid, &status, 0); // Parent waits for foreground processes
        }
    }
}

// Handling pipes and background processes
void handle_pipes_and_execute(char *line) {
    char *commands[MAX_ARGS]; // Commands separated by pipes
    char *args[MAX_ARGS];     // Arguments for each command
    int background = 0;       // Background process flag
    int num_pipes = 0;

    // Split line by "|" to separate commands
    commands[0] = strtok(line, "|");
    while (commands[num_pipes] != NULL) {
        commands[++num_pipes] = strtok(NULL, "|");
    }
    num_pipes--;

    // Set up pipes for multiple commands
    int pipes[num_pipes][2];
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i <= num_pipes; i++) {
        int num_args = tokenize(commands[i], args);

        // Check for background process symbol '&' at the last command
        if (i == num_pipes && strcmp(args[num_args - 1], "&") == 0) {
            background = 1;
            args[num_args - 1] = NULL;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process handling redirection and piping
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < num_pipes) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipes in the child process
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execute(args, background); // Execute the command with redirection and background check
        }
    }

    // Close all pipes in the parent process
    for (int i = 0; i < num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (!background) {
        // Wait for all processes in a pipeline if not a background process
        for (int i = 0; i <= num_pipes; i++) {
            wait(NULL);
        }
    }
}

int main() {
    char line[MAX_LINE];     // Command line
    struct sigaction sa;

    // Set up signal handler for SIGCHLD to handle background process completion
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        read_cmd(line); // Get user input
        handle_pipes_and_execute(line); // Process input with pipes and background handling
    }

    return 0;
}
