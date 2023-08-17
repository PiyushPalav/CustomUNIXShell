# Design of a Custom UNIX Shell

## Simple shell

The shell takes in user input, forks one or more child processes using the fork system call, calls exec from these children to execute user commands, and reaps the dead children using the wait system call. Shell can execute any simple Linux command given as input, e.g., ls, cat, echo and sleep. These commands are readily available as executables on Linux, and the shell simply invokes the corresponding executable, using the user input string as argument to the exec system call.
The shell continues execution indefinitely until the user hits Ctrl+C to terminate the shell. It is assumed that the command to run and its arguments are separated by one or more spaces in the input, so that the input stream can be “tokenized” using spaces as the delimiters. It is assumed that the Linux commands are invoked with simple command-line arguments, and without any special modes of execution like background execution, I/O redirection, or pipes.
Exec is invoked on any command that the user gives as input. If the Linux command execution fails due to incorrect arguments, an error message is printed on screen by the executable, and shell moves on to the next command. If the command itself does not exist, then the exec system call fails, and an error message is printed on screen, and shell moves on to the next command.
It is assumed that the input command has no more than 1024 characters, and no more than 64 tokens. Further, it is also assumed that each token is no longer than 64 characters.
The command cd <directoryname> is implemented using chdir system call and causes the shell process to change its working directory, and cd .. changes to the parent directory. A separate child process is not spawned to execute the chdir system call, but chdir is called from shell itself, because calling chdir from the child will change the current working directory of the child whereas we wish to change the working directory of the main parent shell itself.
When the forked child calls exec to execute a command, the child automatically terminates after the executable completes. However, if the exec system call did not succeed for some reason, the shell ensures that the child is terminated suitably.

## Background execution

if a Linux command is followed by &, the command is executed in the background. That is, the shell starts the execution of the command, and returns to prompt the user for the next input, without waiting for the previous command to complete. The output of the command is printed to the shell as and when it appears.
Unlike in the case of foreground execution, the background processes can be reaped with a time delay. The shell checks for dead children periodically, when it obtains a new user input from the terminal. When the shell reaps a terminated background process, it prints a message Shell: Background process finished to let the user know that a background process has finished.
A generic wait system call can reap and return any dead child. So if we are waiting for a foreground process to terminate and invoke wait, it may reap and return a terminated background process. In that case, we must not erroneously return to the command prompt for the next command, but must wait for the foreground command to terminate as well. To avoid such cases, waitpid variant of wait system call is used, to reap the correct foreground child.

## The exit command

Up until now, the shell executes in an infinite loop, and only the signal SIGINT (Ctrl+C) causes it to terminate. The exit command is implemented that will cause the shell to terminate its infinite loop and exit. When the shell receives the exit command, it terminates all background processes, by sending them a signal via the kill system call. Before exiting, the shell also cleans up any internal state (e.g., free dynamically allocated memory), and terminates in a clean manner.

## Handling the Ctrl+C signal

Until now, The Ctrl+C command causes the shell (and all its children) to terminate. Now, shell is modified so that the signal SIGINT does not terminate the shell itself, but only terminates the foreground process it is running. Note that the background processes remain unaffected by the SIGINT, and only terminate on the exit command. This functionality is achieved by writing custom signal handling code in the shell, that catches the Ctrl+C signal and relays it to the relevant processes, without terminating itself.
Note that, by default, any signal like SIGINT is delivered to the shell and all its children. To handle this, we carefully place the various children of the shell in different process groups, using the setpgid system call. The process group of its children are manipluated to ensure that only the foreground child receives the Ctrl+C signal, and the background children in a separate process group do not get killed by the Ctrl+C signal immediately.
