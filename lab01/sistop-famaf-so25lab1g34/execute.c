#include <assert.h>
#include <fcntl.h>
#include <glib.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "builtin.h"
#include "tests/syscall_mock.h"

void execute_pipeline(pipeline apipe) {
    assert(apipe != NULL);

    if (builtin_alone(apipe)) {
        builtin_run(pipeline_front(apipe));
        return;
    }

    if (pipeline_contains_builtin(apipe)) {
        fprintf(stderr, "mybash: builtins cannot be in a pipeline\n");
        return;
    }

    /* Evita zombies para procesos en background */
    if (!pipeline_get_wait(apipe)) signal(SIGCHLD, SIG_IGN);

    int pids[64];
    int pid_count = 0;
    int prev_pipe = -1;
    scommand command;
    while (!pipeline_is_empty(apipe)) {
        command = pipeline_front(apipe);
        unsigned int length = scommand_length(command);
        char **command_array = malloc((length + 1) * sizeof(char *));
        for (unsigned int i = 0; i < length; i++) {
            command_array[i] = strdup(scommand_front(command));
            scommand_pop_front(command);
        }
        command_array[length] = NULL;
        bool next_pipe = pipeline_length(apipe) > 1;
        int p[2];
        if (next_pipe) {
            if (pipe(p) < 0) {
                printf("Pipe error\n");
                return;
            }
        }
        int fork_result = fork();
        if (fork_result < 0) {
            printf("Fork failed\n");
            return;
        } else if (fork_result == 0) {
            if (scommand_get_redir_in(command) != NULL) {
                int file_in = open(scommand_get_redir_in(command), O_RDONLY, 0);
                if (file_in < 0) {
                    printf("File descriptor error\n");
                    return;
                }
                dup2(file_in, STDIN_FILENO);
                close(file_in);
            } else if (prev_pipe != -1) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }
            if (scommand_get_redir_out(command) != NULL) {
                int file_out = open(scommand_get_redir_out(command), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (file_out < 0) {
                    printf("File descriptor error\n");
                    return;
                }
                dup2(file_out, STDOUT_FILENO);
                close(file_out);
            } else if (next_pipe) {
                close(p[0]);
                dup2(p[1], STDOUT_FILENO);
                close(p[1]);
            }
            execvp(command_array[0], command_array);
            printf("Exec error\n");
        } else {
            pids[pid_count++] = fork_result;
            if (prev_pipe != -1) close(prev_pipe);
            if (next_pipe) {
                close(p[1]);
                prev_pipe = p[0];
            } else {
                prev_pipe = -1;
            }
        }

        for (unsigned int i = 0; i < length; i++) {
            free(command_array[i]);
        }
        free(command_array);

        pipeline_pop_front(apipe);
    }

    if (pipeline_get_wait(apipe)) {
        for (int i = 0; i < pid_count; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }

    return;
}

