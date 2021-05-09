#include "myshell_parser.h"
#include "stddef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct pipeline *pipeline_build(const char *command_line)
{
	// TODO: Implement this function

	struct pipeline * mypipe = (struct pipeline *)malloc(sizeof(struct pipeline));
	struct pipeline_command * currentcom;
    struct pipeline_command * firstcom = (struct pipeline_command *)malloc(sizeof(struct pipeline_command));

	int command_arg_counter = 0;
	unsigned int i = 0, j = 0, k = 0;
	int arg_recorded = 0, arg_iter = 0;
	int redin_recorded = 0, redout_recorded = 0, redin_iter = 0, redout_iter = 0;
	char argument[257] = "";
	char redirectin[100] = "";
	char redirectout[100] = "";


	mypipe->is_background = false;
	
	firstcom->command_args[0] = NULL;
	firstcom->redirect_in_path = NULL;
	firstcom->redirect_out_path = NULL;
	firstcom->next = NULL;

	currentcom = firstcom;
	
	for (i = 0; i < strlen(command_line); i++)
	{
		if (arg_recorded == 1)
		{
			memset(argument,0,sizeof(argument));
			arg_recorded = 0;
			arg_iter = 0;
		}
		if (redin_recorded == 1)
		{
			memset(redirectin,0,sizeof(redirectin));
			redin_recorded = 0;
			redin_iter = 0;
		}
		if (redout_recorded == 1)
		{
			memset(redirectout,0,sizeof(redirectout));
			redout_recorded = 0;
			redout_iter = 0;
		}



		switch (command_line[i])
		{
			case '&': 
				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					strcpy(currentcom->command_args[command_arg_counter],argument);
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}

				mypipe->is_background = true;
				break;
			case '|': 
				  
				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					strcpy(currentcom->command_args[command_arg_counter],argument);
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}
				
				currentcom->next = (struct pipeline_command *)malloc(sizeof(struct pipeline_command));
				currentcom = currentcom->next;
				currentcom->redirect_in_path = NULL;
				currentcom->redirect_out_path = NULL;
				command_arg_counter = 0;
				break;
			case '<': 
				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					strcpy(currentcom->command_args[command_arg_counter],argument);
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}
				for (j = i + 1; j < strlen(command_line); j++)
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
							strncat(redirectin,&command_line[j],1);
							redin_iter++;
						}

						i = j + 1;
					}
				}
 
				currentcom->redirect_in_path = (char *)malloc(strlen(redirectin)*sizeof(char));
				strcpy(currentcom->redirect_in_path,redirectin);
			       	redin_recorded = 1;
				break;
			case '>':

				if (strcmp(argument,"") && arg_recorded == 0)
				{			
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					strcpy(currentcom->command_args[command_arg_counter],argument);
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}


				for (k = i + 1; k < strlen(command_line); k++)
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
							redout_iter++;
						}

						i = k + 1;
					}
				}
 
      			currentcom->redirect_out_path = (char *)malloc(strlen(redirectout)*sizeof(char));
				strcpy(currentcom->redirect_out_path, redirectout);
				redout_recorded = 1;
				break;
			case ' ':
				if (strcmp(argument,"") && arg_recorded == 0)
				{
					currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
					strcpy(currentcom->command_args[command_arg_counter],argument);
					currentcom->command_args[command_arg_counter + 1] = NULL;
					arg_recorded = 1;
					command_arg_counter++;
				}
				break;
			default:

				  if (!isspace(command_line[i]))
				  {
					strncat(argument,&command_line[i],1);
					arg_iter++;
				  }
				  break;

		} /*end switch*/

	} /*end for*/



	if (strcmp(argument,"") && arg_recorded == 0)
	{
		currentcom->command_args[command_arg_counter] = (char *)malloc(strlen(argument)*sizeof(char));
		strcpy(currentcom->command_args[command_arg_counter],argument);
		currentcom->command_args[command_arg_counter + 1] = NULL;
		arg_recorded = 1;
		command_arg_counter++;
	}

	mypipe->commands = firstcom;
	currentcom->next = NULL;

	return mypipe;
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
		current = next;
	}
}
