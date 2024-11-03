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
#define MAX_VARS 100

typedef struct {
    char *str;     // name=value string
    int global;    // Boolean: 1 if global (environment), 0 if local
} Var;

typedef struct {
    pid_t pid;
    char command[256];
} Job;

Job jobs[MAX_JOBS];
Var vars[MAX_VARS];   // Array to store variables
int job_count = 0;
int var_count = 0;

// Function prototypes
int execute(char *arglist[], int input_fd, int output_fd, int error_fd, int background);
int handle_builtin(char *arglist[]);
char **tokenize(char *cmdline, int *background);
void handle_pipes_and_execute(char **arglist, int background);
void sigchld_handler(int signum);
void display_prompt(char *prompt);
void add_job(pid_t pid, const char *command);
void list_jobs();
void kill_job(pid_t pid);
int are_jobs_present();

// Variable handling functions
int set_var(const char *name, const char *value, int global);
void unset_var(const char *name);
const char *get_var(const char *name);
void list_vars();
void expand_variables(char **arglist);

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

        if (!cmdline) break;  // Exit on EOF

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
            expand_variables(arglist);  // Expand variables in the command

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
    snprintf(prompt, PATH_MAX + 50, "\n-(%s)-[%s]\n---$ ", PROMPT, cwd);
}

// Function to set a variable (local or global)
int set_var(const char *name, const char *value, int global) {
    // Check if variable already exists
    for (int i = 0; i < var_count; i++) {
        if (strncmp(vars[i].str, name, strlen(name)) == 0 && vars[i].str[strlen(name)] == '=') {
            free(vars[i].str);
            vars[i].str = malloc(strlen(name) + strlen(value) + 2);
            sprintf(vars[i].str, "%s=%s", name, value);
            vars[i].global = global;
            if (global) setenv(name, value, 1);  // Update environment variable
            return 1;
        }
    }

    // Add new variable if space is available
    if (var_count < MAX_VARS) {
        vars[var_count].str = malloc(strlen(name) + strlen(value) + 2);
        sprintf(vars[var_count].str, "%s=%s", name, value);
        vars[var_count].global = global;
        if (global) setenv(name, value, 1);
        var_count++;
        return 1;
    } else {
        fprintf(stderr, "Variable storage limit reached.\n");
        return 0;
    }
}

// Function to unset a variable 
void unset_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        // Check if the variable matches the name
        if (strncmp(vars[i].str, name, strlen(name)) == 0 && vars[i].str[strlen(name)] == '=') {
            free(vars[i].str); // Free the memory allocated for the variable string

            // Shift remaining variables up
            for (int j = i; j < var_count - 1; j++) {
                vars[j] = vars[j + 1];
            }

            var_count--; // Decrement the variable count
            printf("Variable %s unset.\n", name);
            return;
        }
    }
    printf("Variable %s not found.\n", name);
}

// Function to get the value of a variable
const char *get_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strncmp(vars[i].str, name, strlen(name)) == 0 && vars[i].str[strlen(name)] == '=') {
            return vars[i].str + strlen(name) + 1;  // Return value part of name=value
        }
    }
    return getenv(name);  // Check environment variables if not found in locals
}

// Function to list all variables
void list_vars() {
    if (var_count == 0) {
        printf("No variables to Display!");
    } else {
        printf("Local and environment variables:\n");
        for (int i = 0; i < var_count; i++) {
            printf("%s (%s)\n", vars[i].str, vars[i].global ? "global" : "local");
        }
    }
}

// Function to expand variables in the argument list
void expand_variables(char **arglist) {
    for (int i = 0; arglist[i] != NULL; i++) {
        if (arglist[i][0] == '$') {
            const char *value = get_var(arglist[i] + 1);
            if (value) {
                free(arglist[i]);
                arglist[i] = strdup(value);
            }
        }
    }
}

// Extended handle_builtin to add variable-related commands
int handle_builtin(char *arglist[]) {
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing operand\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
        return 1;
    } else if (strcmp(arglist[0], "exit") == 0) {
        printf("Exit Successfully!\n");
        exit(0);
    } else if (strcmp(arglist[0], "set") == 0) {
        if (arglist[1] && arglist[2]) {
            set_var(arglist[1], arglist[2], 0);  // Set local variable
        } else {
            printf("Usage: set <variable> <value>\n");
        }
        return 1;
    } else if (strcmp(arglist[0], "unset") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "unset: missing variable name\n");
        } else {
            unset_var(arglist[1]);
        }
        return 1;
    } else if (strcmp(arglist[0], "export") == 0) {
        if (arglist[1] && arglist[2]) {
            set_var(arglist[1], arglist[2], 1);  // Set global (environment) variable
        } else {
            printf("Usage: export <variable> <value>\n");
        }
        return 1;
    } else if (strcmp(arglist[0], "get") == 0) {
        if (arglist[1]) {
            const char *value = get_var(arglist[1]);
            if (value) {
                printf("%s=%s\n", arglist[1], value);
            } else {
                printf("%s not found.\n", arglist[1]);
            }
        } else {
            printf("Usage: get <variable>\n");
        }
        return 1;
    } else if (strcmp(arglist[0], "list") == 0) {
        list_vars();
        return 1;
    }  else if (strcmp(arglist[0], "jobs") == 0) {
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
            fprintf(stderr, "kill: missing PID or job ID\n");
        } else {
            int signal = SIGKILL;  // Default signal is SIGKILL
            int target_pid = -1;   // PID to kill
            int job_id = -1;       // Initialize job ID as -1 (not found)
            
            // Check if a signal is provided (e.g., kill -9 <job_id/PID>)
            if (arglist[1][0] == '-') {
                signal = atoi(arglist[1] + 1);
                job_id = atoi(arglist[2]);  // Job ID or PID as the next argument
            } else {
                job_id = atoi(arglist[1]);  // Treat arglist[1] as job ID or PID
            }

            // Attempt to find the PID by job ID first
            if (job_id > 0 && job_id <= job_count && jobs[job_id - 1].pid != 0) {
                target_pid = jobs[job_id - 1].pid;  // Retrieve PID from job ID
            } else {
                target_pid = job_id;  // If no job match, assume it's a PID
            }

            // Kill the process using the target PID
            if (target_pid <= 0 || kill(target_pid, signal) != 0) {
                perror("kill");
            } else {
                printf("Killed %s [%d] %d\n", (job_id == target_pid ? "process" : "job"), job_id, target_pid);
                // Mark the job as terminated if killed by job ID
                if (job_id <= job_count && job_id > 0 && jobs[job_id - 1].pid == target_pid) {
                    jobs[job_id - 1].pid = 0;
                }
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
