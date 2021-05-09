#include "myshell_parser.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>

int main(int argc, char* argv[])
{
    //Variables for retrieving user input
    char *input = NULL;
    int checkget = 0;
    size_t buffer = 0;

    //Variables for verifying valid input
    int input_valid = 0;
    int ampersand_present = 0;
    int background_valid = -1;

    struct pipeline* my_pipeline = (struct pipeline *)malloc(sizeof(struct pipeline));
    struct pipeline_command * current;
    struct pipeline_command * next;
    int redirectin_valid = -1;
    int redirectout_valid = -1;
    int redirectin_count = 0;
    int redirectout_count = 0;
    int command_count = 0;
    int count_rein_carrot = 0;
    int count_reout_carrot = 0;


    while(1)
    {
        if (argc == 2 && !strcmp(argv[1], "-n"))
        {
            checkget = getline(&input, &buffer, stdin);
        }
        else
        {
            printf("my_shell$");
            checkget = getline(&input, &buffer, stdin);
        }

        if (checkget == -1)     //check if input is ctrl-D
        {
            exit(0);
        }

        //check if '&' is at end (background validity)
        for (int i = 0; i < strlen(input); i++)
        {
            if (input[i] == '&')
            {
                ampersand_present = 1;
                for (int j = i + 1; j < strlen(input); j++)
                {
                    if (isspace(input[j]))
                    {
                        background_valid = 1;
                    }
                    else
                    {
                        background_valid = 0;
                        break;
                    }
                }
                break;
            }
        }

        if (ampersand_present == 1 && background_valid == 0)
        {
            printf("ERROR: Invalid input. '&' must be at end.\n");
        }

        //check for redirectin/redirectout validity
        my_pipeline = pipeline_build(input);
        current = my_pipeline->commands;

        while (current != NULL)        //check for redirectin validity
        {
             next = current->next;
             command_count++;
            if (current->redirect_in_path != NULL)
            {
                redirectin_count++;

                if (command_count == 1 && redirectin_count == 1)
                {
                    redirectin_valid = 1;
                }
                else if (command_count > 1 && redirectin_count == 1)
                {
                    redirectin_valid = 0;
                }
                else if (redirectin_count > 1)
                {
                    redirectin_valid = 0;
                }
            }
            current = next;
        }

         for (int i = 0; i < strlen(input); i++)
         {
            if (input[i] == '<')
            {
               count_rein_carrot++;
            }

            if (count_rein_carrot > 1)
            {
                redirectin_valid = 0;
                break;
            }
         }

        if (redirectin_valid == 0)
        {
            printf("ERROR: Invalid input. Redirect-in can only be used once and only used in first command\n");
        }


        current = my_pipeline->commands;
        while (current != NULL)        //check for redirectout validity
        {
            next = current->next;
            if (current->redirect_out_path != NULL)
            {
                redirectout_count++;

                if (current->next == NULL && redirectout_count == 1)
                {
                    redirectout_valid = 1;
                }
                else if (current->next != NULL && redirectout_count == 1)
                {
                    redirectout_valid = 0;
                }
                else if (redirectout_count > 1)
                {
                    redirectout_valid = 0;
                }
            }
            current = next;
        }

        for (int i = 0; i < strlen(input); i++)
        {
            if (input[i] == '>')
            {
               count_reout_carrot++;
            }

            if (count_reout_carrot > 1)
            {
                redirectout_valid = 0;
                break;
            }
        }   

        if (redirectout_valid == 0)
        {
            printf("ERROR: Invalid input. Redirect-out can only be used once and only used in last command\n");
        }




        if (redirectin_valid == 0 || redirectout_valid == 0 || background_valid == 0)
        {
            input_valid = 0;
        }
        else
        {
            input_valid = 1;

            execute(input);           //proceed to execute current input
        }

        memset(input,0,sizeof(input));
        background_valid = -1;
        ampersand_present = 0;
        pipeline_free(my_pipeline);
        free(my_pipeline);
        my_pipeline = (struct pipeline *)malloc(sizeof(struct pipeline));
        redirectin_count = 0;
        redirectin_valid = -1;
        command_count = 0;
        redirectout_count = 0;
        redirectout_valid = -1;
        count_rein_carrot = 0;
        count_reout_carrot = 0;

    }
}

void execute(char* input)
{
    struct pipeline* my_pipeline = (struct pipeline *)malloc(sizeof(struct pipeline));
    struct pipeline_command * current;
    struct pipeline_command * next;

    //Variables for fork
    pid_t pid, pidpipe;
    int status;

    //Variables for pipe
    int fds[2], in = 0;
    int pipeval = 0;
    int num_commands = 1;
    int bkg = 0;

    //Variables for error checking
    int errorWithRedIn = 0;

    my_pipeline = pipeline_build(input);

    for (int n = 0; n < strlen(input); n++)
    {
        if (input[n] == '|')
        {
            num_commands++;
        }

        if (input[n] == '&')
        {
            bkg = 1;
        }
    }


    if (num_commands > 1)   //for pipes
    {
        current = my_pipeline->commands;

        pid = fork();

        if (pid == 0)   //Child 1
        {
            for (int p = 0; p < num_commands - 1; ++p)
            {
                next = current->next;
                pipeval = pipe(fds);

                if ((pidpipe = fork()) == 0)   //Child 2
                {
                    if (in != 0)                //if 'in' is not stdin
                    {
                        dup2(in,0);
                        close(in);
                    }

                    if (fds[1] != 1)            //if fds[1] is not stdout
                    {
                        dup2(fds[1],1);
                        close(fds[1]);
                    }



                    if (current->redirect_out_path != NULL)       //working so far
                    {            
                        int fd1 = open(current->redirect_out_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                        dup2(fd1, 1);
                        close(fd1);
                    }
                    if (current->redirect_in_path != NULL)        //working so far
                    {
                        int fd2 = open(current->redirect_in_path, O_RDONLY);
                        if (fd2 == -1)
                        {
                            perror("ERROR");
                            errorWithRedIn = 1;
                            exit(1);
                        }
                        else
                        {
                            dup2(fd2,0);
                            close(fd2);
                        }
                    }
                    if (errorWithRedIn == 0)
                    {
                        execvp(current->command_args[0],current->command_args);
                    }
                }
                //Parent 2
                waitpid(-1, &status, 0);    
                close(fds[1]);
                in = fds[0];                    //preserve read end of pipe for next command/child

                current = next;
            }

            if (in != 0)
            {
                dup2(in,0);                     //set 'in' to read end
            }
            current = next;

            if (current->redirect_out_path != NULL)       //working so far
            {            
                int fd1 = open(current->redirect_out_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd1, 1);
                close(fd1);
            }
            if (current->redirect_in_path != NULL)        //working so far
            {
                int fd2 = open(current->redirect_in_path, O_RDONLY);
                if (fd2 == -1)
                {
                    perror("ERROR");
                    errorWithRedIn = 1;
                    exit(0);
                }
                else
                {
                    dup2(fd2,0);
                    close(fd2);
                }
            }
            if (errorWithRedIn == 0)
            {
                execvp(current->command_args[0],current->command_args);         //execute last command
            }
        }
        else
        {
            if (bkg == 0)
            {
                waitpid(-1, &status, 0);
            }
        }
    }       //end if for pipes
    else    // for single commands
    {
        pid = fork();

        if (pid == 0)
        {
            if (my_pipeline->commands->redirect_out_path != NULL)       //working so far
            {            
                int fd1 = open(my_pipeline->commands->redirect_out_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd1, 1);
                close(fd1);
            }
            if (my_pipeline->commands->redirect_in_path != NULL)        //working so far
            {
                int fd2 = open(my_pipeline->commands->redirect_in_path, O_RDONLY);
                if (fd2 == -1)
                {
                    perror("ERROR");
                    errorWithRedIn = 1;
                }
                else
                {
                    dup2(fd2,0);
                    close(fd2);
                }
            }
            if (errorWithRedIn == 0)
            {
                execvp(my_pipeline->commands->command_args[0],my_pipeline->commands->command_args);
            }
        }
        else
        {
            if (bkg  == 0)
            {
                waitpid(-1,&status,0);
            }
        }
    }
    pipeline_free(my_pipeline);
    free(my_pipeline);
    memset(input,0,sizeof(input));
    my_pipeline = (struct pipeline *)malloc(sizeof(struct pipeline));
    num_commands = 1;
    bkg = 0;
    errorWithRedIn = 0;
}