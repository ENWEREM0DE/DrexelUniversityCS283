1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

My implementation uses a loop to call waitpid() on each child process ID that was saved during the fork operations. This ensures all children complete before returning to the prompt. If I forgot to call waitpid(), zombie processes would accumulate in the system, causing resource leaks. Additionally, the shell might prompt for new input before previous commands finish, leading to unpredictable behavior and output interleaving.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

After dup2() copies a file descriptor, the original descriptor remains open and must be closed to prevent file descriptor leaks. If unused pipe ends aren't closed, processes won't receive EOF signals when expected because the pipe still has open write ends. This can cause processes to hang indefinitely waiting for input that will never arrive, particularly in pipelines where processes expect EOF to know when their input is complete.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command must be built-in because it needs to change the current working directory of the shell process itself. If implemented as an external command, it would change the directory of the child process only, which terminates after execution, leaving the parent shell's directory unchanged. This would make directory navigation impossible since changes wouldn't persist between commands.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

I would replace the fixed-size array with a dynamically allocated linked list or resizable array of command structures. The implementation would allocate memory as needed when parsing the command line and free it after execution. Trade-offs include increased complexity for memory management, potential for memory leaks if not carefully implemented, and slightly higher overhead for managing dynamic structures. However, this approach would provide flexibility for handling commands of arbitrary complexity.