For this assignment, we implemented a simple shell that interprets the meta-characters '<', '>', '|', and '&'. It operates on a REPL structure: reading an input from the user, evaluating the input (determining if it's valid and correctly executing it), printing the output to the screen, and looping the process.

When coding, the site linuxhint.com was useful in helping me understand various system calls. There are helpful explanations and examples. I used the following pages:
https://linuxhint.com/exec_linux_system_call_c/
https://linuxhint.com/fork_linux_system_call_c/
https://linuxhint.com/pipe_system_call_c/
https://linuxhint.com/dup2_system_call_c/

I also used this site to help me understand I/O redirection better: 
https://linuxcommand.org/lc3_lts0070.php

_______________________________________________________________________________________
Description of program:

In myshell_parser.c:

In my code, I start off by allocating space for the first 'pipeline_command' of the pipeline that will be returned by the function. All members of this command is initially set to 'NULL', and 'is_background' is set to 'false'.

For my code, I have a for-loop that iterates through each character of the command line. Inside the for loop is a switch statement that has a case for each type of token as well as a case for spaces. For the '&' case, it sets the value of 'is_background' to 'true'. The cases for '<' and '>' each have an inner for-loop that reads through the next argument to retrieve the redirect-in or redirect-out source/destination. At the end if these inner for-loops, the value of 'i' (which controlls the outer for-loop mentioned earlier), is updated. For the '|' case, memory is allocated for a new 'pipeline_command' and '*next' pointer of the current 'pipeline_command' points to this new allocated memory. This becomes the new current command. The default case in the switch statement is for any characters that are not tokens or spaces. Each time this case is taken, it appends the current character to a character array called 'argument'. 

Additionally, in every other case, it checks if the current 'argument' has been stored as one of the 'command_args'. If not, then the argument is stored into 'command_args' using the strcpy() function. Once the argument is stored, the variable 'argument' is cleared using memset(), so that it is ready to retrieve the next argument in the command. This same method is used for the redirect-in and redirect-out values. 

After the outer for-loop is exited, my code checks if a final argument needs to be recorded in 'command_args', and it records the argument if needed. Finally, it returns the pipeline at the end of the function.

For my pipeline_free function, I free the memory use in each 'pipeline_command' similar to how one would free the memory in a singly linekd list. I use a pointer to 'pipeline_command' called 'current' that points to the first command of the pipeline, and another pointer called 'next' that points to the command following the 'current' command. Then, I used a while-loop to navigate through the pipeline. For each command, it frees 'command_args' and the values for 'redirect_in_path' and 'redirect_out_path' if their values are not 'NULL'. The 'current' pointer is set to the command that 'next' is pointing to, and the while-loop runs until 'current' points to 'NULL', indicating that the end of the pipeline has been reached.

In myshell.c:

There is a main function and an 'execute' function. The main function first prompts the user for an input then checks to make sure the input command is valid. It does this by making sure that the '&' (if there is one) is at the end of the line. Then it makes sure that there is at most one '<' and ath most one '<'. It checks that '>' is only present in the last command and that '<' is only present in the first command. 

After it has been confirmed that the input is valid, the main function then calls 'execute', which is in charge of exectuing the command(s). If the input is invalid, then an error message is printed and the user is prompted for a new input. 

The 'execute' function has a block for running pipelines and a separate block for running single commands. In the pipeline block, a loop is used to execute each command, and a pipe is used to continue passing the stdout of one command to the stdin of the next command. Inside the loop, forks are used to execute each command.

The block for running single commands is very similar to the pipeline block, except it only uses fork() once. In both blocks, before calling execvp(), the code checks if there is a redirect-in-path and redirect-out-path. If either exists, dup2() is used to set the file path to either stdout or stdin.