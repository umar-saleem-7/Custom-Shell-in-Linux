# MyShell - Version 1

`MYshell` is a lightweight, custom shell program built in C. This shell demonstrates essential features of a command-line interface, such as displaying a prompt, accepting commands, parsing them, and executing them in a child process. Additionally, it supports a controlled exit using `<CTRL+D>`.

## Features

   - The prompt displays as `MYshell:- <current-directory> $`, showing the current working directory for user context.
   - `MYshell` allows users to enter commands with arguments in a single line.
   - It forks a child process to execute the command using `execvp`, and the parent process waits for the child process to complete.
   - Users can quit `MYshell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.
