#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_CMDS 16

char **split_pipes(char *line, int *count);

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


void handle_pipes(char *line)
{
    int cmd_count;

    char **cmds = split_pipes(line, &cmd_count);

    int prev_fd = -1;

    for (int i = 0; i < cmd_count; i++)
    {
        // Pipe = kernel buffer that allows two processes to communicate with each other
        // create a access points to that buffer with two file descriptors: fd[0] for reading, fd[1] for writing
        int fd[2];

        // create pipe except for last command
        if (i < cmd_count - 1)
        {
            if (pipe(fd) == -1)
            {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            // INPUT from previous pipe
            if (prev_fd != -1)
            {
                // redirect stdin to the read end of the pipe
                // input is no longer coming from keyboard, but from the pipe buffer
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            // OUTPUT to next pipe
            if (i < cmd_count - 1)
            {

                // redirect stdout to the write end of the pipe
                // output is no longer going to the terminal, but to the pipe buffer
                dup2(fd[1], STDOUT_FILENO);

                close(fd[0]);
                close(fd[1]);
            }

            char **args = tokenize(cmds[i]);

            execvp(args[0], args);

            perror("execvp");
            exit(1);
        }

        // parent cleanup
        if (prev_fd != -1)
        {
            close(prev_fd);
        }

        if (i < cmd_count - 1)
        {
            close(fd[1]);
            prev_fd = fd[0];
        }
    }

    // wait all children
    for (int i = 0; i < cmd_count; i++)
    {
        wait(NULL);
    }

    free(cmds);
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

char **split_pipes(char *line, int *count)
{
    char **cmds = malloc(MAX_CMDS * sizeof(char *));
    char *cmd;
    int i = 0;

    cmd = strtok(line, "|");

    while (cmd != NULL && i < MAX_CMDS - 1)
    {
        while (*cmd == ' ')
            cmd++; // trim leading spaces

        cmds[i++] = cmd;

        cmd = strtok(NULL, "|");
    }

    cmds[i] = NULL;
    *count = i;

    return cmds;
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
            handle_pipes(input);
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