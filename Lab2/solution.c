#include "parser.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>


char **create_commands(const struct command *cmd) {
    char **argv = malloc((2 + cmd->arg_count) * sizeof(char *));
    argv[0] = cmd->exe;
    for (uint32_t i = 0; i < cmd->arg_count; ++i) {argv[i + 1] = cmd->args[i];}
    argv[cmd->arg_count + 1] = NULL;
    return argv;
}

static void execute_command_line(const struct command_line *line) {
    const struct expr *ex = line->head;

    if (ex->type == EXPR_TYPE_COMMAND && !strncmp(ex->cmd.exe, "exit", 4) && ex->next == NULL) {
        int ret = 0;
        if (ex->cmd.arg_count) {ret = atoi(ex->cmd.args[0]);}
        exit(ret);
    }
    int dup_stdout = dup(STDOUT_FILENO);
    int dup_stdin = dup(STDIN_FILENO);
    int file = dup(STDOUT_FILENO);

    if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
        file = open(line->out_file, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        dup2(file, STDOUT_FILENO);
    } else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
        file = open(line->out_file, O_RDWR | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
        dup2(file, STDOUT_FILENO);
    }


    bool in_pipe_left = false;
    bool in_pipe_right = false;
    int fd_l[2] = {-1, -1};
    int fd_r[2] = {-1, -1};

    while (ex != NULL) {
        if (ex->type == EXPR_TYPE_COMMAND) {
            if (!strncmp(ex->cmd.exe, "cd", 4)) {
                chdir(*ex->cmd.args);
                ex = ex->next;
                continue;
            }
            if (ex->next && ex->next->type == EXPR_TYPE_PIPE) {
                in_pipe_left = true;
            } else {
                in_pipe_left = false;
            }
            if (in_pipe_right) {
                dup2(fd_r[0], STDIN_FILENO);
                close(fd_l[1]);
            } else {
                dup2(dup_stdin, STDIN_FILENO);
                close(fd_r[0]);
            }
            if (in_pipe_left) {
                pipe(fd_l);
                dup2(fd_l[1], STDOUT_FILENO);
            } else {
                dup2(file, fd_r[1]);
                dup2(file, STDOUT_FILENO);
            }

            pid_t pid = fork();
            if (pid == 0) {
                char **argv = create_commands(&ex->cmd);
                execvp(ex->cmd.exe, argv);
                free(argv);
            } else if (pid > 0) {
                wait(NULL);
            }
        } else if (ex->type == EXPR_TYPE_PIPE) {
            in_pipe_right = true;
            if (fd_r[0]) {
                close (fd_r[0]);
            }
            if (fd_r[1]) {
                close (fd_r[1]);
            }
            fd_r[0] = dup(fd_l[0]);

            close(fd_l[0]);
            close(fd_l[1]);
            fd_l[0] = -1;
            fd_l[1] = -1;
        } else {}
        ex = ex->next;
        dup2(dup_stdout, STDOUT_FILENO);
        dup2(dup_stdin, STDIN_FILENO);
    }

    dup2(dup_stdin, STDIN_FILENO);
    dup2(dup_stdout, STDOUT_FILENO);
    close(dup_stdin);
    close(dup_stdout);

}


int
main(void)
{
    const size_t buf_size = 1024;
    char buf[buf_size];
    int rc;
    struct parser *p = parser_new();
    while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
        parser_feed(p, buf, rc);
        struct command_line *line = NULL;
        while (true) {
            enum parser_error err = parser_pop_next(p, &line);
            if (err == PARSER_ERR_NONE && line == NULL)
                break;
            if (err != PARSER_ERR_NONE) {
                printf("Error: %d\n", (int)err);
                continue;
            }
            execute_command_line(line);
            command_line_delete(line);
        }
    }
    parser_delete(p);
    return 0;
}