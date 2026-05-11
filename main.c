#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 64

char **tokenize(char *line)
{
    char **args = malloc(MAX_ARGS * sizeof(char *));
    char *token;
    int i = 0;

    token = strtok(line, " ");

    while (token != NULL && i < MAX_ARGS - 1)
    {
        // args[i] = strdup(token);
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        i++;

        token = strtok(NULL, " ");
    }

    args[i] = NULL;

    return args;
}

void free_args(char **args)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        free(args[i]);
    }
    free(args);
}

int main()
{
    char input[1024];
    while (1)
    {
        printf("mini-shell> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            return 1;
        }

        input[strcspn(input, "\n")] = '\0';
        
        char **args = tokenize(input);

        // for (int i = 0; args[i] != NULL; i++) {
        //     printf("arg[%d] = %s\n", i, args[i]);
        // }

        if (strcmp(args[0], "exit") == 0)
        {
            free_args(args);
            return 0;
        }

        else if (strcmp(args[0], "cd") == 0)
        {
            const char *path = args[1] ? args[1] : getenv("HOME");
            if (path == NULL)
            {
                fprintf(stderr, "cd: expected argument\n");
            }
            else
            {
                if (chdir(path) != 0)
                {
                    perror("cd");
                }
            }
        }

        else
        {
            pid_t pid = fork();

            if (pid == 0)
            {
                execvp(args[0], args);
                perror("execvp");
                exit(1);
            }
            else
            {
                wait(NULL);
            }
        }
        // Free memory
        free_args(args);
    }

    return 0;
}