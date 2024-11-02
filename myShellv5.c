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
#include <errno.h>
#include <limits.h>

#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "MyShell"
#define HISTORY_FILE ".my_shell_history"
#define MAX_JOBS 100

typedef struct {
    pid_t pid;
    char command[256];
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;

int execute(char *arglist[], int input_fd, int output_fd, int error_fd, int background);
int handle_builtin(char *arglist[]);
char **tokenize(char *cmdline, int *background);
void handle_pipes_and_execute(char **arglist, int background);
void sigchld_handler(int signum);
void display_prompt(char *prompt);
void add_job(pid_t pid, const char *command);
void list_jobs();
void kill_job(pid_t pid);
int are_jobs_present(); // Function declaration for checking job presence

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    using_history();
    read_history(HISTORY_FILE);

    char *cmdline;
    char **arglist;
    char prompt[PATH_MAX + 50];

    while (1) {
        display_prompt(prompt);
        cmdline = readline(prompt);

        if (!cmdline) {  // Exit on EOF
            break;
        }

        int background = 0;
        if (cmdline[0] != '\0') {
            add_history(cmdline);
            write_history(HISTORY_FILE);
        }

        // Handle command repetition with `!number` and `!!`
        if (cmdline[0] == '!') {
            HIST_ENTRY *entry = NULL;
            if (cmdline[1] == '!') {  // Repeat the last command
                entry = previous_history();
            } else {
                int history_index = atoi(cmdline + 1) - 1;
                if (history_index >= 0 && history_index < history_length) {
                    entry = history_get(history_index + 1);
                }
            }

            if (entry) {
                free(cmdline);
                cmdline = strdup(entry->line);
                printf("Repeating command: %s\n", cmdline);
            } else {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }
        }

        if ((arglist = tokenize(cmdline, &background)) != NULL) {
            if (handle_builtin(arglist) == 0) {  // If it's a built-in command
                handle_pipes_and_execute(arglist, background);  // External command handling
            }

            for (int j = 0; j < MAXARGS + 1; j++) {
                free(arglist[j]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    write_history(HISTORY_FILE);
    printf("\n");
    return 0;
}

void display_prompt(char *prompt) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        strcpy(cwd, "unknown");
    }
    snprintf(prompt, PATH_MAX + 50, "(%s)-[%s]:- ", PROMPT, cwd);
}

int handle_builtin(char *arglist[]) {
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing operand\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
        return 1;
    } else if (strcmp(arglist[0], "exit") == 0) {
        printf("Exit Successfully!");
        exit(0);
    } else if (strcmp(arglist[0], "jobs") == 0) {
        if (are_jobs_present()) { // Check for job presence
            list_jobs(); // List the jobs if present
            return 1;
        } else {
            // If no jobs are present
            printf("No jobs Found.\n");
            return -1; // Return value for no jobs
        }
    } else if (strcmp(arglist[0], "kill") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "kill: missing PID or job id\n");
        } else {
            int signal = SIGKILL;
            int pid;

            // Check if signal is provided, e.g., kill -9 <pid>
            if (arglist[1][0] == '-') {
                signal = atoi(arglist[1] + 1);
                pid = atoi(arglist[2]);
            } else {
                pid = atoi(arglist[1]);
            }

            if (pid <= 0 || kill(pid, signal) != 0) {
                perror("kill");
            }
        }
        return 1;
    } else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd [directory] - change directory\n");
        printf("  exit - exit the shell\n");
        printf("  jobs - list background jobs\n");
        printf("  kill [-signal] <pid> - send a signal to a process\n");
        printf("  help - display this help message\n");
        return 1;
    }
    return 0;
}

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
            add_job(cpid, arglist[0]);
            printf("[%d] %d\n", job_count, cpid);
            return 0;
        } else {
            int status;
            waitpid(cpid, &status, 0);
            return status;
        }
    }
}

void add_job(pid_t pid, const char *command) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, command, 255);
        jobs[job_count].command[255] = '\0';
        job_count++;
    } else {
        fprintf(stderr, "Job list full, cannot add more jobs.\n");
    }
}

void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid != 0) {
            printf("[%d] %d %s\n", i + 1, jobs[i].pid, jobs[i].command);
        }
    }
}

void kill_job(int job_id) {
    if (job_id < 1 || job_id > job_count || jobs[job_id - 1].pid == 0) {
        fprintf(stderr, "kill: invalid job id %d\n", job_id);
    } else {
        if (kill(jobs[job_id - 1].pid, SIGKILL) == 0) {
            printf("Killed job [%d] %d\n", job_id, jobs[job_id - 1].pid);
            jobs[job_id - 1].pid = 0;  // Mark job as terminated
        } else {
            perror("kill");
        }
    }
}

void sigchld_handler(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                jobs[i].pid = 0;  // Mark as terminated
                break;
            }
        }
    }
}

char **tokenize(char *cmdline, int *background) {
    char **arglist = (char **)malloc(sizeof(char *) * (MAXARGS + 1));
    if (!arglist) {
        perror("malloc() failed for arglist");
        return NULL;
    }

    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char *)malloc(sizeof(char) * ARGLEN);
        if (!arglist[j]) {
            perror("malloc() failed for arglist element");
            return NULL;
        }
        bzero(arglist[j], ARGLEN);
    }

    int argnum = 0;
    char *cp = cmdline;
    char *start;
    int len;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t') cp++;
        start = cp;
        len = 1;
        while (*++cp != '\0' && *cp != ' ' && *cp != '\t') len++;
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

void handle_pipes_and_execute(char **arglist, int background) {
    int input_fd = -1, output_fd = -1, error_fd = -1;
    int pipefd[2];

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
            if (pipe(pipefd) == -1) {
                perror("pipe() failed");
                return;
            }

            arglist[i] = NULL;
            execute(arglist, input_fd, pipefd[1], error_fd, 0);

            close(pipefd[1]);
            input_fd = pipefd[0];
            arglist = &arglist[i + 1];
            i = -1;
        }
    }

    execute(arglist, input_fd, output_fd, error_fd, background);

    if (input_fd != -1) close(input_fd);
    if (output_fd != -1) close(output_fd);
    if (error_fd != -1) close(error_fd);
}

int are_jobs_present() {
    // Check if there are any active jobs
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid != 0) {
            return 1; // Job is present
        }
    }
    return 0; // No jobs present
}
