/*myShellv1.c
-main() receive a string and pass it to tokenize
-tokenize allocate the dynamic memory and tokenize the string by space and return a char**
-main() pass this tokenize string to execute function which then fork() and exec()
-main() display for new command untill <ctrl+d> is pressed*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <strings.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "MyShell:- "

int execute(char *arglist[]);
char **tokenize(char *cmdline);
char *read_cmd(char *, FILE *);

int main() {
    char *cmdline;
    char **arglist;
    char *prompt = PROMPT;
    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        if ((arglist = tokenize(cmdline)) != NULL) {
            execute(arglist);
            for (int j = 0; j < MAXARGS + 1; j++)
                free(arglist[j]);
            free(arglist);
            free(cmdline);
        }
    }
    printf("\n");
    return 0;
}

int execute(char *arglist[]) {
    int status;
    pid_t cpid = fork();
    switch (cpid) {
        case -1:
            perror("fork() failed");
            exit(1);
        case 0:
            execvp(arglist[0], arglist);
            perror("!...command not found...!");
            exit(1);
        default:
            waitpid(cpid, &status, 0);
            printf("child exited with status %d \n", status >> 8);
            return 0;
    }
}

char **tokenize(char *cmdline) {
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
    arglist[argnum] = NULL;
    return arglist;
}

char *read_cmd(char *prompt, FILE *fp) {
    printf("%s", prompt);
    int c;
    int pos = 0;
    char *cmdline = (char *)malloc(sizeof(char) * MAX_LEN);
    while ((c = getc(fp)) != EOF) {
        if (c == '\n')
            break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0)
        return NULL;
    cmdline[pos] = '\0';
    return cmdline;
}
