#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "MyShell:- "

int execute(char *arglist[], int input_fd, int output_fd, int error_fd, int background);
char **tokenize(char *cmdline);
char *read_cmd(char *prompt, FILE *fp);
void handle_pipes_and_execute(char **arglist, int background);

int main() {
    char *cmdline;
    char **arglist;
    char *prompt = PROMPT;

    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        if ((arglist = tokenize(cmdline)) != NULL) {
            handle_pipes_and_execute(arglist, 0); // Default background to 0
            for (int j = 0; j < MAXARGS + 1; j++)
                free(arglist[j]);
            free(arglist);
            free(cmdline);
        }
    }
    printf("\n");
    return 0;
}

// Execute a single command with optional redirection
int execute(char *arglist[], int input_fd, int output_fd, int error_fd, int background) {
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork() failed");
        exit(1);
    }
    if (cpid == 0) { // Child process
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
    } else { // Parent process
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

// Tokenize the command line input into arguments
char **tokenize(char *cmdline) {
    char **arglist = (char **)malloc(sizeof(char *) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char *)malloc(sizeof(char) * ARGLEN);
    }
    
    if (cmdline[0] == '\0') {
        free(arglist);
        return NULL;
    }

    int argnum = 0;
    char *cp = cmdline;
    char *start;
    int len;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t') {
            cp++;
        }
        start = cp;
        len = 0;
        while (*cp != '\0' && *cp != ' ' && *cp != '\t') {
            len++;
            cp++;
        }
        if (len > 0) {
            strncpy(arglist[argnum], start, len);
            arglist[argnum][len] = '\0'; // Null terminate the string
            argnum++;
        }
    }
    arglist[argnum] = NULL; // Null terminate the argument list
    return arglist;
}

// Read command input
char *read_cmd(char *prompt, FILE *fp) {
    printf("%s", prompt);
    int c;
    int pos = 0;
    char *cmdline = (char *)malloc(sizeof(char) * MAX_LEN);
    if (!cmdline) {
        perror("malloc failed");
        exit(1);
    }
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') {
            break;
        }
        cmdline[pos++] = c;
        if (pos >= MAX_LEN - 1) { // Prevent buffer overflow
            break;
        }
    }
    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL;
    }
    cmdline[pos] = '\0';
    return cmdline;
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
            i++; // Move past the file name
        } else if (strcmp(arglist[i], ">") == 0) {
            output_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd < 0) {
                perror("Failed to open output file");
                return;
            }
            free(arglist[i]);
            arglist[i] = NULL;
            i++; // Move past the file name
        } else if (strcmp(arglist[i], "2>") == 0) {
            error_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (error_fd < 0) {
                perror("Failed to open error file");
                return;
            }
            free(arglist[i]);
            arglist[i] = NULL;
            i++; // Move past the file name
        } else if (strcmp(arglist[i], "|") == 0) {
            has_pipe = 1;
            pipe(pipefd);
            arglist[i] = NULL; // Null terminate the first command
            execute(arglist, input_fd, pipefd[1], error_fd, 0); // Execute command before pipe
            close(pipefd[1]);
            input_fd = pipefd[0]; // Set input for the next command
            arglist = &arglist[i + 1]; // Move to next command
            i = -1; // Reset loop to start over with new arglist
        }
    }

    // Execute the final command with redirection
    execute(arglist, input_fd, output_fd, error_fd, background);

    if (input_fd != -1) close(input_fd);
    if (output_fd != -1) close(output_fd);
    if (error_fd != -1) close(error_fd);
}
