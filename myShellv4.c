#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "MyShell:- "
#define HISTORY_SIZE 10

int execute(char *arglist[], int input_fd, int output_fd, int error_fd, int background);
char **tokenize(char *cmdline, int *background);
void handle_pipes_and_execute(char **arglist, int background);
void sigchld_handler(int signum);
void add_to_history(const char *cmd);
void save_history();

char *history[HISTORY_SIZE];
int history_count = 0;

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, 0);

    using_history();  // Initialize readline history
    read_history(".my_shell_history");  // Load history from file

    char *cmdline;
    char **arglist;

    while ((cmdline = readline(PROMPT)) != NULL) {  // Use readline to capture input with history support
        int background = 0;

        // Add non-empty command to history
        if (cmdline[0] != '\0') {
            add_history(cmdline);  // Add to readline's internal history
            add_to_history(cmdline);  // Add to our own history for !number handling
        }

        // Handle command repetition with `!number`
        // Handle command repetition with `!number`
        if (cmdline[0] == '!') {
            int history_index;

            if (cmdline[1] == '-') {
                history_index = history_count - 1; // Last command
            } else {
                history_index = atoi(cmdline + 1) - 1; // Get command number
            }

            // Ensure the history index is valid
            if (history_index >= 0 && history_index < history_count) {
                free(cmdline);
                cmdline = strdup(history[history_index]); // Use history command
                printf("Repeating command: %s\n", cmdline);
            } else {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }
        }


        if ((arglist = tokenize(cmdline, &background)) != NULL) {
            handle_pipes_and_execute(arglist, background);

            for (int j = 0; j < MAXARGS + 1; j++)
                free(arglist[j]);
            free(arglist);
            free(cmdline);
        }
    }
    
    // Save history to a file for future sessions
    save_history();
    
    printf("\n");
    return 0;
}

// Add command to history, using a circular buffer of fixed size
void add_to_history(const char *cmd) {
    if (history_count == HISTORY_SIZE) {
        free(history[0]);
        memmove(history, history + 1, (HISTORY_SIZE - 1) * sizeof(char *));
        history_count--;
    }
    history[history_count++] = strdup(cmd);
}

// Save history to a file
void save_history() {
    for (int i = 0; i < history_count; i++) {
        free(history[i]); // Free previous history entries
    }
    write_history(".my_shell_history");
}

// Execute a single command with optional redirection and background execution
int execute(char *arglist[], int input_fd, int output_fd, int error_fd, int background) {
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork() failed");
        exit(1);
    }
    if (cpid == 0) {
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        if (error_fd != -1) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }

        execvp(arglist[0], arglist);
        perror("!...command not found...!");
        exit(1);
    } else {
        if (background) {
            printf("[%d] %d\n", 1, cpid);
            return 0;
        } else {
            int status;
            waitpid(cpid, &status, 0);
            return status;
        }
    }
}

// Tokenize the command line input into arguments, detecting background flag
char **tokenize(char *cmdline, int *background) {
    char **arglist = (char **)malloc(sizeof(char *) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char *)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[j], ARGLEN);
    }
    if (cmdline[0] == '\0')
        return NULL;

    int argnum = 0;
    char *cp = cmdline;
    char *start;
    int len;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t')
            cp++;
        start = cp;
        len = 1;
        while (*++cp != '\0' && *cp != ' ' && *cp != '\t')
            len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum > 0 && strcmp(arglist[argnum - 1], "&") == 0) {
        *background = 1;
        free(arglist[argnum - 1]);
        arglist[argnum - 1] = NULL;
    }
    arglist[argnum] = NULL;
    return arglist;
}

// Handle pipes and redirection logic
void handle_pipes_and_execute(char **arglist, int background) {
    int input_fd = -1, output_fd = -1, error_fd = -1;
    int pipefd[2];
    int has_pipe = 0;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            input_fd = open(arglist[i + 1], O_RDONLY);
            if (input_fd < 0) {
                perror("Failed to open input file");
                return;
            }
            free(arglist[i]);
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], ">") == 0) {
            output_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd < 0) {
                perror("Failed to open output file");
                return;
            }
            free(arglist[i]);
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], "2>") == 0) {
            error_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (error_fd < 0) {
                perror("Failed to open error file");
                return;
            }
            free(arglist[i]);
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], "|") == 0) {
            has_pipe = 1;
            pipe(pipefd);

            arglist[i] = NULL;
            execute(arglist, input_fd, pipefd[1], error_fd, 0);

            close(pipefd[1]);
            input_fd = pipefd[0];
            arglist = &arglist[i + 1];
            i = -1;
        }
    }

    // Execute the final command with redirection
    execute(arglist, input_fd, output_fd, error_fd, background);

    if (input_fd != -1) close(input_fd);
    if (output_fd != -1) close(output_fd);
    if (error_fd != -1) close(error_fd);
}

// Signal handler for SIGCHLD to clean up terminated background processes
void sigchld_handler(int signum) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}
