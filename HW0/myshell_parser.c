#include "myshell_parser.h"
#include "stddef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct pipeline *pipeline_build(const char *command_line)
{
	// TODO: Implement this function

	//struct pipeline * returnedpipe = (struct pipeline *)malloc(sizeof(struct pipeline));
	struct pipeline * mypipe = (struct pipeline *)malloc(sizeof(struct pipeline));
	struct pipeline_command * currentcom;
        struct pipeline_command * firstcom = (struct pipeline_command *)malloc(sizeof(struct pipeline_command));

	int command_arg_counter = 0;
	unsigned int i = 0, j = 0, k = 0;
	int arg_recorded = 0;
	int redin_recorded = 0, redout_recorded = 0;
	int redin_exit = 0, redout_exit = 0;
	//char redirectin[100] = "", redirectout[100] = "";
	//char argument[257] = "";
	//char *argument = malloc(sizeof(257*sizeof(char)));
	//char *redirectin = malloc(sizeof(100*sizeof(char)));
	//char *redirectout = malloc(sizeof(100*sizeof(char)));
	char argument[257] = "";
	char redirectin[100] = "";
	char redirectout[100] = "";


	//initializing pointers to NULL//
	mypipe->commands = NULL;
	mypipe->is_background = false;
	

	for (size_t n = 0; n < MAX_ARGV_LENGTH; n++)
	{
		firstcom->command_args[n] = NULL;
	}
	firstcom->redirect_in_path = NULL;
	firstcom->redirect_out_path = NULL;
	firstcom->next = NULL;


	currentcom = firstcom;
	
	for (i = 0; i < strlen(command_line); i++)
	{
		if (arg_recorded == 1)
		{
			//argument = malloc(sizeof(257*sizeof(char)));
			memset(argument,0,sizeof(argument));
			arg_recorded = 0;
			//arg_iter = 0;
		}
		if (redin_recorded == 1)
		{
			//redirectin = malloc(sizeof(100*sizeof(char)));
			memset(redirectin,0,sizeof(redirectin));
			redin_recorded = 0;
			//redin_iter = 0;
		}
		if (redout_recorded == 1)
		{
			//redirectout = malloc(sizeof(100*sizeof(char)));
			memset(redirectout,0,sizeof(redirectout));
			redout_recorded = 0;
			//redout_iter = 0;
		}



		switch (command_line[i])
		{
			case '&': 
				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					//currentcom->command_args[command_arg_counter] = argument;
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					//strcpy(currentcom->command_args[command_arg_counter],argument);
					strncpy(currentcom->command_args[command_arg_counter],argument,strlen(argument));
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}

				mypipe->is_background = true;
				break;
			case '|': 
				  
				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					//currentcom->command_args[command_arg_counter] = argument;
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					//strcpy(currentcom->command_args[command_arg_counter],argument);
					strncpy(currentcom->command_args[command_arg_counter],argument,strlen(argument));
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}
				
				currentcom->next = (struct pipeline_command *)malloc(sizeof(struct pipeline_command));
				currentcom = currentcom->next;
				for (size_t m = 0; m < MAX_ARGV_LENGTH; m++)
				{
					currentcom->command_args[m] = NULL;
				}
				currentcom->redirect_in_path = NULL;
				currentcom->redirect_out_path = NULL;
				command_arg_counter = 0;
				break;
			case '<': 
				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					//currentcom->command_args[command_arg_counter] = argument;
					currentcom->command_args[command_arg_counter] = malloc(strlen(argument)*sizeof(char));
					//strcpy(currentcom->command_args[command_arg_counter],argument);
					strncpy(currentcom->command_args[command_arg_counter],argument,strlen(argument));
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}
				for (j = i + 1; j < strlen(command_line) && redin_exit == 0; j++)
				{
					if (command_line[j] == '&' || command_line[j] == '|' || command_line[j] == '<' || command_line[j] == '>')
					{
						i = j - 1;
						break;
					}
					else
					{
						if (!isspace(command_line[j]))
						{
							//*(redirectin + redin_iter) = *(command_line + j);
							strncat(redirectin,&command_line[j],1);
							//redin_iter++;
						}
						
						if (redirectin[0] != '\0' && isspace(command_line[j]))
						{
							redin_exit = 1;
							j--;
						}

						i = j + 1;
					}
				}
 
				//currentcom->redirect_in_path = redirectin;
				currentcom->redirect_in_path = (char *)malloc((strlen(redirectin) + 1)*sizeof(char));
				//strcpy(currentcom->redirect_in_path,redirectin);
				strncpy(currentcom->redirect_in_path,redirectin,strlen(redirectin));
			       	redin_recorded = 1;
				redin_exit = 0;
				break;
			case '>':

				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					//currentcom->command_args[command_arg_counter] = argument;
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					//strcpy(currentcom->command_args[command_arg_counter],argument);
					strncpy(currentcom->command_args[command_arg_counter],argument,strlen(argument));
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}


				for (k = i + 1; k < strlen(command_line) && redout_exit == 0; k++)
				{
					if (command_line[k] == '&' || command_line[k] == '|' || command_line[k] == '<' || command_line[k] == '>')
					{
						i = k - 1;
						break;
					}
					else
					{
						if (!isspace(command_line[k]))
						{
							strncat(redirectout,&command_line[k],1);
							//*(redirectout + redout_iter) = *(command_line + k);
							//redout_iter++;
						}

						if (redirectout[0] != '\0' && isspace(command_line[k]))
						{
							redout_exit = 1;
							k--;
						}

						i = k + 1;
					}
				 }
 
				 //currentcom->redirect_out_path = redirectout;
      				 currentcom->redirect_out_path = (char *)malloc(strlen(redirectout)*sizeof(char));
				// strcpy(currentcom->redirect_out_path, redirectout);
				 strncpy(currentcom->redirect_out_path,redirectout,strlen(redirectout));
				 redout_recorded = 1;
				 redout_exit = 0;
				 break;
			case ' ':
				if (strcmp(argument,"") && arg_recorded == 0)
				{
					//currentcom->command_args[command_arg_counter] = argument;

					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					//strcpy(currentcom->command_args[command_arg_counter],argument);
					strncpy(currentcom->command_args[command_arg_counter],argument,strlen(argument));
					//free(currentcom->command_args[command_arg_counter]);
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}
				break;
			default:

				  if (!isspace(command_line[i]))
				  {
				  	//strcat(argument,(command_line + i));
					//*(argument + arg_iter) = *(command_line + i);
					strncat(argument,&command_line[i],1);
					//arg_iter++;
				  }
				  break;

		} /*end switch*/

	} /*end for*/


	if (strcmp(argument,"") && arg_recorded == 0)
	{
		//*(currentcom->command_args[command_arg_counter]) = argument;
		currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
		//strcpy(currentcom->command_args[command_arg_counter],argument);
		strncpy(currentcom->command_args[command_arg_counter],argument,strlen(argument));
		currentcom->command_args[command_arg_counter + 1] = NULL;
		arg_recorded = 1;
		command_arg_counter++;
	}

	mypipe->commands = firstcom;
	currentcom->next = NULL;

	//free(currentcom);
	//returnedpipe = &mypipe;
	//return returnedpipe;
	return mypipe;
	//return NULL;
}

void pipeline_free(struct pipeline *pipeline)
{
	// TODO: Implement this function
	
	struct pipeline_command * current = pipeline->commands;
	struct pipeline_command * next;
	int i = 0;

	while (current != NULL)
	{
		next = current->next;
		i = 0;
		while (current->command_args[i] != NULL)
		{
			free(current->command_args[i]);
			i++;
		}
		if (current->redirect_in_path != NULL)
		{			
			free(current->redirect_in_path);
		}
		if (current->redirect_out_path != NULL)
		{
			free(current->redirect_out_path);
		}
		free(current);
		current = next;
	}
}

