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

int execute(char *arglist[], int input_fd, int output_fd);
char **tokenize(char *cmdline);
char *read_cmd(char *, FILE *);
void handle_pipes_and_execute(char **arglist);

int main() {
    char *cmdline;
    char **arglist;
    char *prompt = PROMPT;

    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        if ((arglist = tokenize(cmdline)) != NULL) {
            handle_pipes_and_execute(arglist);
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
int execute(char *arglist[], int input_fd, int output_fd) {
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork() failed");
        exit(1);
    }
    if (cpid == 0) {
        // If input redirection is specified
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        // If output redirection is specified
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        execvp(arglist[0], arglist);
        perror("!...command not found...!");
        exit(1);
    } else {
        int status;
        waitpid(cpid, &status, 0);
        return status;
    }
}
