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
