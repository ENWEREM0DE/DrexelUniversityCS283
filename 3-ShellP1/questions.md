1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  I think fork() is essential because it creates a copy of the current process, allowing the shell to continue running while the command executes in the child process. Using fork() prevents the shell from being replaced by the executed command, which would happen with execvp() alone.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  I think fork() failing is a critical error that typically indicates the system is out of resources or has reached its process limit. My implementation reports the error via perror(), sets the last_return_code to errno, and returns ERR_EXEC_CMD to the caller.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: I think execvp() uses the PATH environment variable to search for the command in each directory listed in PATH, in order. It's important to note that execvp() first tries the command as-is, then prepends each PATH directory until it finds an executable match.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  I think wait() is crucial because it prevents zombie processes by allowing the parent to collect the child's exit status and resources. Not calling wait() would leave terminated child processes as zombies, consuming system resources.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  I think WEXITSTATUS() is important because it extracts the actual exit code (0-255) from the status value returned by wait(), allowing us to determine if the command succeeded. This lets us track and report command execution results accurately.
6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  I think my build_cmd_buff() implementation carefully tracks quoted strings by maintaining a in_quotes state and preserving spaces within quotes. This allows users to include spaces in arguments by quoting them, like echo "hello world".

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: I think the biggest change was moving from a command list structure to a single command buffer, which simplified the code but required careful refactoring of the parsing logic. I also improved the quote handling and space collapsing behavior.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  I think signals are lightweight, asynchronous notifications used to tell processes about events, unlike other IPC methods that transfer data. It's important to note that signals interrupt normal program flow to handle immediate events.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?
    
    > **Answer**: 
    
    SIGTERM (15) This is the standard termination signal that asks a process to shut down gracefully, allowing it to clean up resources. This is typically used when you want to give a process a chance to save data and exit normally.
   
    SIGKILL (9): This is a forceful termination signal that cannot be caught or ignored by the process. This is used when a process is completely unresponsive and needs to be stopped immediately.
     
     SIGINT (2): This is the interrupt signal sent when a user presses Ctrl+C in the terminal. This is commonly used to allow users to interactively stop programs during development or normal operation.



- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**: SIGSTOP is special because it cannot be caught or ignored, always suspending process execution until a SIGCONT is received. This behavior is essential for reliable process suspension in job control.