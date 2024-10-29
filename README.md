# MyShell - Version 1 - Custom shell programmed in C

`MYshell` is a lightweight, custom shell program built in C. This shell demonstrates essential features of a command-line interface, such as displaying a prompt, accepting commands, parsing them, and executing them in a child process. Additionally, it supports a controlled exit using `<CTRL+D>`.

## Features

   - The prompt displays as `MYshell:- <current-directory> $`, showing the current working directory for user context.
   - `MYshell` allows users to enter commands with arguments in a single line.
   - It forks a child process to execute the command using `execvp`, and the parent process waits for the child process to complete.
   - Users can quit `MYshell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.
# MyShell - Version 2 - Enhanced Custom Shell with I/O Redirection and Pipes

`MYshell` v2 is an upgraded version of previous custom shell programmed in C, extending the basic functionality to include I/O redirection and command piping. This version allows users to execute commands with enhanced control over file descriptors, making it a more powerful tool for command-line interactions.

## Features
   - The prompt displays as `MyShell:- <current-directory>:-`, showing the current working directory and helping users keep track of their environment.
   - `MYshell` v2 allows users to enter commands with arguments in a single line.
   - It forks a child process to execute the command using `execvp`, with the parent process waiting for the child process to complete.
   - Supports input (`<`) and output (`>`) redirection for commands.
   - Users can specify files for standard input and output, enabling commands like `mycmd < infile > outfile`.
   - The shell opens `infile` in read-only mode and `outfile` in write-only mode, using `dup2()` to redirect file descriptors.
   - `MYshell` v2 enables piping (`|`) between commands, allowing the output of one command to be used as the input for another.
   - Supports multiple pipes in a single command, allowing complex command chaining.
   - Users can quit `MyShell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.

# MYshell - Version 3- Custom Shell with Background Execution and Zombie Handling

`MYshell` v3 introduces background process execution, allowing users to run commands in the background by appending `&` at the end of the command line. This feature enables asynchronous command execution, letting users continue using the shell immediately after starting a background process. Additionally, this version includes signal handling to prevent zombie processes.

## Features

   - The prompt displays as `MyShell:- <current-directory>:-`, showing the current working directory to help users keep track of their location.
   - `MYshell` v3 allows users to enter commands with arguments and options, and it forks a child process to execute the command.
   - For standard commands, the shell waits for the child process to complete. For background commands (commands ending with `&`), the shell returns to the prompt immediately, allowing users to run multiple commands simultaneously.
   - Users can run external commands in the background by appending `&` at the end of the command, like so:
     ```plaintext
     mycmd &
     ```
   - This lets users continue working in the shell while `mycmd` executes independently.
   - To avoid zombie processes, `MYshell` v3 uses signal handling to capture the termination of background processes.
   - The shell uses `sigaction()` to set up a signal handler for `SIGCHLD`, which automatically reaps finished background processes, keeping the process table clean.
   - Users can exit `MYshell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.

