1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  I think fgets() is ideal because it safely reads full lines and cleanly handles EOF, making it robust for a shell. 

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  I needed to use malloc() because the size of the input can vary, and a fixed-size array would not be flexible. 


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  If we didn't trim spaces, the command would be stored with leading and trailing spaces, which could cause issues when executing the command. 

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  We should implement command > file for redirecting output, command < file for receiving input from a file, and command 2> file for sending error output to a file. I think the challenges include correctly managing file descriptors and ensuring that overlapping or multiple redirections don’t interfere with each other.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  Redirection is used to control the input and output of a command, while piping is used to connect the output of one command to the input of another command. 

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  It is important to keep these separate in a shell because it allows for better error handling and debugging. 

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:   I think our shell should display errors via STDERR but also let users merge them with STDOUT (using something like 2>&1) if they choose.