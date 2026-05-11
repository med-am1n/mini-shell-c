#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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

void handle_pipe(char *line)
{
    char *pipe_pos = strchr(line, '|');

    if (pipe_pos != NULL)
    {
        *pipe_pos = '\0'; // split string into two parts

        char *left = line;
        char *right = pipe_pos + 1;

        char **left_args = tokenize(left);
        char **right_args = tokenize(right);

        // Pipe = kernel buffer that allows two processes to communicate with each other
        // create a access points to that buffer with two file descriptors: fd[0] for reading, fd[1] for writing
        // dup2() to redirect stdin/stdout to the pipe ends
        int fd[2];

        if (pipe(fd) == -1)
        {
            perror("pipe");
            return;
        }

        pid_t pid1 = fork();

        if (pid1 == 0)
        {
            // LEFT side → writes to pipe
            // redirect stdout to the write end of the pipe
            // output is no longer going to the terminal, but to the pipe buffer
            dup2(fd[1], STDOUT_FILENO);

            close(fd[0]);
            close(fd[1]);

            execvp(left_args[0], left_args);
            perror("execvp left failed");
            exit(1);
        }

        pid_t pid2 = fork();

        if (pid2 == 0)
        {
            // RIGHT side → reads from pipe
            // redirect stdin to the read end of the pipe
            // input is no longer coming from keyboard, but from the pipe buffer
            dup2(fd[0], STDIN_FILENO);

            close(fd[0]);
            close(fd[1]);

            execvp(right_args[0], right_args);
            perror("execvp right failed");
            exit(1);
        }

        // Parent
        close(fd[0]);
        close(fd[1]);

        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);

        free_args(left_args);
        free_args(right_args);
    }
}

void handle_redirection(char **args)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        // OUTPUT REDIRECTION >
        if (strcmp(args[i], ">") == 0)
        {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0)
            {
                perror("open");
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);

            args[i] = NULL; // terminate command
        }

        // APPEND >>
        else if (strcmp(args[i], ">>") == 0)
        {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0)
            {
                perror("open");
                exit(1);
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);

            args[i] = NULL;
        }

        // INPUT REDIRECTION <
        else if (strcmp(args[i], "<") == 0)
        {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0)
            {
                perror("open");
                exit(1);
            }

            dup2(fd, STDIN_FILENO);
            close(fd);

            args[i] = NULL;
        }
    }
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
        
        // PIPE FIRST
        if (strchr(input, '|') != NULL)
        {
            handle_pipe(input);
            continue;
        }

        // NORMAL PATH
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
                handle_redirection(args);

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