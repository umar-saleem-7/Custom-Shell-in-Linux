# MyShell - Version 1 - Custom shell programmed in C

`MyShell` is a lightweight, custom shell program built in C. This shell demonstrates essential features of a command-line interface, such as displaying a prompt, accepting commands, parsing them, and executing them in a child process. Additionally, it supports a controlled exit using `<CTRL+D>`.

## Features

   - The prompt displays as `MyShell:- `, showing the current working directory for user context.
   - `MYshell` allows users to enter commands with arguments in a single line.
   - It forks a child process to execute the command using `execvp`, and the parent process waits for the child process to complete.
   - Users can quit `MYshell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.
# MyShell - Version 2 - Enhanced Custom Shell with I/O Redirection and Pipes

`MyShell` v2 is an upgraded version of previous custom shell programmed in C, extending the basic functionality to include I/O redirection and command piping. This version allows users to execute commands with enhanced control over file descriptors, making it a more powerful tool for command-line interactions.

## Features
   - The prompt displays as `MyShell:- `, showing the current working directory and helping users keep track of their environment.
   - `MYshell` v2 allows users to enter commands with arguments in a single line.
   - It forks a child process to execute the command using `execvp`, with the parent process waiting for the child process to complete.
   - Supports input (`<`), output (`>`) and error (`2>`) redirection for commands.
   - Users can specify files for standard input and output, enabling commands like `mycmd < infile > outfile 2> errorfile`.
   - The shell opens `infile` in read-only mode, `outfile` in write-only mode and `errorfile` in write- mode or append mode, using `dup2()` to redirect file descriptors.
   - `MyShell` v2 enables piping (`|`) between commands, allowing the output of one command to be used as the input for another.
   - Supports multiple pipes in a single command, allowing complex command chaining.
   - Users can quit `MyShell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.

# MyShell - Version 3- Custom Shell with Background Execution and Zombie Handling

`MyShell` v3 introduces background process execution, allowing users to run commands in the background by appending `&` at the end of the command line. This feature enables asynchronous command execution, letting users continue using the shell immediately after starting a background process. Additionally, this version includes signal handling to prevent zombie processes.

## Features

   - The prompt displays as `MyShell:- `, showing the current working directory to help users keep track of their location.
   - `MYshell` v3 allows users to enter commands with arguments and options, and it forks a child process to execute the command.
   - For standard commands, the shell waits for the child process to complete. For background commands (commands ending with `&`), the shell returns to the prompt immediately, allowing users to run multiple commands simultaneously.
   - Users can run external commands in the background by appending `&` at the end of the command, like so:
     ```plaintext
     mycmd &
     ```
   - This lets users continue working in the shell while `mycmd` executes independently.
   - To avoid zombie processes, `MyShell` v3 uses signal handling to capture the termination of background processes.
   - The shell uses `sigaction()` to set up a signal handler for `SIGCHLD`, which automatically reaps finished background processes, keeping the process table clean.
   - Users can exit `MYshell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.

# MyShell - Version4 - Custom Shell with Command History and Recall

`MyShell` v4 enhances the custom shell with a command history feature, allowing users to repeat previously issued commands by referencing their command numbers. This version supports commands like `!number`, where `number` corresponds to the command's position in the history list. It maintains a history of the last 10 commands and overwrites older entries as new commands are added.

## Features

   - The prompt displays as `MyShell:- `, indicating the current working directory.
   - `MyShell` v4 accepts commands with options and arguments, forking a child process to execute them.
   - The shell keeps track commands issued by the user.
   - Users can repeat commands by typing `!number` (e.g., `!!` for the last command or `!1` for the first command in the history).
   - For advanced users, you can implement support for command navigation using the up and down arrow keys through the `readline()` library.
   - Users can exit `MYshell` by pressing `<CTRL+D>`, allowing a smooth exit from the program.

# MyShell - Version 5 - Custom Shell with Built-in Commands

`MyShell` v5 enhances the custom shell to include built-in commands that provide essential functionalities directly within the shell. Unlike external commands, built-in commands are part of the shell's code, allowing for quicker execution and integrated features.

## Features

   - The prompt displays as `(MyShell) - [cwd]-$`, indicating the current working directory.
   - The shell supports several built-in commands that can be executed without forking a new process:
     - **`cd <directory>`**: Changes the current working directory to the specified directory.
     - **`exit`**: Terminates the shell session and exits the program.
     - **`jobs`**: Displays a numbered list of background processes currently executing.
     - **`kill <job_number> / PID`**: Terminates the specified background process by sending it a `SIGKILL` signal.
     - **`help`**: Lists all available built-in commands and their usage.
   - For external commands, the shell searches for the executable in the system's path, forks a child process, and uses `exec()` to run the command.
   - Users can exit `MyShell` by typing `exit` or pressing `<CTRL+D>`, allowing a smooth termination of the session.

# MyShell - Version 6 - Custom Shell with Variable Support

`MyShell` v6 introduces variable support, enabling users to define and manage both user-defined (local) and environment variables. These variables can be assigned values, retrieved, and listed within the shell environment. The shell distinguishes between local and global variables, storing variable data in a structure that supports efficient access and modification.

## Features
   - The prompt displays as `(myShell)-[cwd]-$ `, indicating the current working directory.

   - This version of `MYshell` supports two types of variables:
     - **Local (User-defined) Variables**: Variables defined by the user, available only within the shell session.
     - **Environment (Global) Variables**: System-wide variables available to the shell and any child processes.
   - Variables are stored in an internal table, represented by an array of structs. Each struct contains:
     - `char *str`: Stores the variable name and value as a `name=value` string.
     - `int global`: Boolean flag to indicate if a variable is global (environment) or local.

   - **Assignment**: Variables can be assigned values using the `set` syntax.
     ```plaintext
     set MyVar value
     ```
   - **Retrieval**: Retrieve variable values by prefixing with `$` or `get`.
     ```plaintext
     echo $VAR_NAME
     ```
     ```plaintext
     get VAR_NAME
     ```
   - **Listing Variables**: Display all stored variables, their values and their scope (local or global) using
     ```plaintext
     list
     ```

   - The shell parses commands to distinguish between variable assignments and other commands. It identifies and manages variable operations before passing external commands to be executed.

   - Users can exit `MYshell` by typing `exit` or pressing `<CTRL+D>`, allowing a smooth termination of the session.
