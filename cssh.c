#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab
#define _DEFAULT_SOURCE // required for strsep() on cslab
#define _BSD_SOURCE // required for strsep() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MAX_ARGS 32


char **get_next_command(size_t *num_args)
{
    // print the prompt
    printf("cssh$ ");

    // get the next line of input
    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, stdin);
    if (ferror(stdin))
    {
        perror("getline");
        exit(1);
    }
    if (feof(stdin))
    {
        return NULL;
    }

    // turn the line into an array of words
    char **words = (char **)malloc(MAX_ARGS*sizeof(char *));
    int i=0;

    char *parse = line;
    while (parse != NULL)
    {
        char *word = strsep(&parse, " \t\r\f\n");
        if (strlen(word) != 0)
        {
            words[i++] = strdup(word);
        }
    }
    *num_args = i;
    for (; i<MAX_ARGS; ++i)
    {
        words[i] = NULL;
    }

    // all the words are in the array now, so free the original line
    free(line);

    return words;
}



//method to ensure no more than two redirections of the same type
int error_check(char **command_list)
{
	int index = 0,input = 0,output = 0,errors = 0;
	while (command_list[index] != NULL)
	{
		if ((strcmp(command_list[index], ">") == 0) || (strcmp(command_list[index], ">>") == 0))
		{       
			output += 1;
			if (output == 2)
			{       
				printf("Error! Can't have two >'s or >>'s!\n");
				errors = 1;
			}
		}
		if (strcmp(command_list[index], "<") == 0)
		{
			input += 1;
			if (input == 2)
			{
				printf("Error! Can't have two <'s!\n");
				errors = 1;
			}
		}
		index++;
	}
	return errors;
}

void free_command(char **words)
{
    for (int i=0; i<MAX_ARGS; ++i)
    {
        if (words[i] == NULL)
        {
            break;
        }
        free(words[i]);
    }
    free(words);
}


void infile_handle(char *filename)
{
    int read_fd = open(filename, O_RDONLY);
    if (read_fd == -1)
    {
        perror("open");
        exit(1);
    }
    // dup read_fd onto stdin
    if (dup2(read_fd, STDIN_FILENO) == -1)
    {
        perror("dup2");
        exit(1);
    }
    close(read_fd);
}

void outfile_tunc_handle(char *filename)
{
    int write_fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (write_fd == -1)
    {
        perror("open");
        exit(1);
    }
    // dup write_fd onto stdout
    if (dup2(write_fd, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        exit(1);
    }
    close(write_fd);
}

void outfile_append_handle(char *filename)
{
    int write_fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (write_fd == -1)
    {
        perror("open");
        exit(1);
    }
    // dup write_fd onto stdout
    if (dup2(write_fd, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        exit(1);
    }
    close(write_fd);
}


int main()
{
    size_t num_args;
    // get the next command
    char **command_line_words = get_next_command(&num_args);
    while (command_line_words != NULL)
    {
        int errors = error_check(command_line_words);
		if (errors == 1)
		{
            free_command(command_line_words);
            command_line_words = get_next_command(&num_args);
            continue;
		}

        if (command_line_words[0] == NULL)
		{
			free_command(command_line_words);
			command_line_words = get_next_command(&num_args);
			continue;
		}

        if (num_args ==1 && strcmp(command_line_words[0],"exit")== 0)
	    {
            free_command(command_line_words);
	        return 1;
	    }


        int i = 0;
        int append_option = 0;

        pid_t fork_rv = fork();
	    if (fork_rv == -1)
	    {
	        perror("fork");
	        exit(1);
	    }
        if (fork_rv == 0)
	    {
            while(command_line_words[i] != NULL)
            {
                if ((strcmp(command_line_words[i], ">") == 0) || (strcmp(command_line_words[i], ">>") == 0))
                    {
                        if (strcmp(command_line_words[i], ">>") == 0)
                        {
                            append_option = 1;
                        }
                        char *filename = command_line_words[i+1];
                        if(append_option == 1)
                        {
                            outfile_append_handle(filename);
                            command_line_words[i] = NULL;
                        }
                        else
                        {
                            outfile_tunc_handle(filename);
                            command_line_words[i] = NULL;
                        }
                    }
                    if (command_line_words[i] == NULL)
                    {
                        i++;
                        continue;
                    }
                    else if (strcmp(command_line_words[i], "<") == 0)
                    {
                        char *filename = command_line_words[i+1];
                        infile_handle(filename);
                        command_line_words[i] = NULL;
                    }
                i++;
            }

            execvp(command_line_words[0],command_line_words);
            perror("execvp");
            exit(1);
        }

        if (waitpid(fork_rv, NULL, 0) == -1)    
	    {
            perror("wait");
            exit(1);
    	}
        
        free_command(command_line_words);
        command_line_words = get_next_command(&num_args);
    }
    // free the memory for the last command
    free_command(command_line_words);

    return 0;

}


