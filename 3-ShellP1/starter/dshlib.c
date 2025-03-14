#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/**** 
 **** FOR REMOTE SHELL USE YOUR SOLUTION FROM SHELL PART 3 HERE
 **** THE MAIN FUNCTION CALLS THIS ONE AS ITS ENTRY POINT TO
 **** EXECUTE THE SHELL LOCALLY
 ****
 */

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    char *current = cmd_line;
    int argc = 0;
    bool in_quotes = false;
    char *token_start = NULL;

    clear_cmd_buff(cmd_buff);
    
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    current = cmd_buff->_cmd_buffer;
    
    while (*current && isspace((unsigned char)*current)) {
        current++;
    }
    
    if (!*current) {
        return WARN_NO_CMDS;
    }
    
    token_start = current;
    
    while (*current && argc < CMD_ARGV_MAX - 1) {
        if (*current == '"') {
            if (!in_quotes) {
                if (token_start == current) {
        
                    token_start = current + 1;
                }
                in_quotes = true;
            } else {
               
                in_quotes = false;
                *current = '\0';
                cmd_buff->argv[argc++] = token_start;
                token_start = current + 1;
            }
        } else if (isspace((unsigned char)*current) && !in_quotes) {
         
            if (token_start != current) {
                *current = '\0';
                cmd_buff->argv[argc++] = token_start;
            }
            
            while (isspace((unsigned char)*(current + 1))) {
                current++;
            }
            token_start = current + 1;
        }
        current++;
    }
    
 
    if (token_start && *token_start && argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[argc++] = token_start;
    }
    
 
    cmd_buff->argv[argc] = NULL;
    cmd_buff->argc = argc;
    
    
    if (in_quotes) {
        return ERR_CMD_ARGS_BAD;
    }
    
    return OK;
}

int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        execvp(cmd->argv[0], cmd->argv);
        switch(errno) {
            case ENOENT:
                fprintf(stderr, "%s: command not found\n", cmd->argv[0]);
                break;
            case EACCES:
                fprintf(stderr, "%s: permission denied\n", cmd->argv[0]);
                break;
            case ENOEXEC:
                fprintf(stderr, "%s: execution error\n", cmd->argv[0]);
                break;
            case ENOMEM:
                fprintf(stderr, "%s: out of memory\n", cmd->argv[0]);
                break;
            default:
                fprintf(stderr, "%s: unknown error\n", cmd->argv[0]);
        }
        exit(errno); 
    } else if (pid < 0) {
        perror("fork failed");
        cmd->last_return_code = errno;
        return ERR_EXEC_CMD;
    }

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid failed");
        cmd->last_return_code = errno;
        return ERR_EXEC_CMD;
    }

    if (WIFEXITED(status)) {
        cmd->last_return_code = WEXITSTATUS(status);
        if (cmd->last_return_code != 0) {
            return ERR_EXEC_CMD;
        }
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "Command terminated by signal %d\n", WTERMSIG(status));
        cmd->last_return_code = 128 + WTERMSIG(status);
        return ERR_EXEC_CMD;
    }

    return OK;
}

int exec_pipe_cmd(char *cmd_line) {
    int num_cmds = 0;
    char *cmds[CMD_MAX];
    char *token;
    char *cmd_copy;
    int i, j;
    int status;
    int pipes[CMD_MAX-1][2];
    pid_t pids[CMD_MAX];

    // Make a copy of cmd_line 
    cmd_copy = strdup(cmd_line);
    if (!cmd_copy) {
        return ERR_MEMORY;
    }

    // Split by pipe and count commands
    token = strtok(cmd_copy, "|");
    while (token != NULL && num_cmds < CMD_MAX) {
        // Trim whitespace
        while (*token && isspace((unsigned char)*token)) token++;
        if (*token) {  // Skip empty commands
            cmds[num_cmds] = strdup(token);
            num_cmds++;
        }
        token = strtok(NULL, "|");
    }
    free(cmd_copy);

    if (num_cmds == 0) {
        return WARN_NO_CMDS;
    }

    if (num_cmds > CMD_MAX) {
        printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        return ERR_TOO_MANY_COMMANDS;
    }

    // Create pipes
    for (i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            for (i = 0; i < num_cmds; i++) free(cmds[i]);
            return ERR_EXEC_CMD;
        }
    }

    // Create child processes
    for (i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            for (j = 0; j < num_cmds; j++) free(cmds[j]);
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {  // Child process
            // Set up pipe input
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }

            // Set up pipe output
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds
            for (j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute command
            char *args[CMD_ARGV_MAX];
            int arg_count = 0;
            char *arg = strtok(cmds[i], " \t");
            
            while (arg != NULL && arg_count < CMD_ARGV_MAX - 1) {
                args[arg_count++] = arg;
                arg = strtok(NULL, " \t");
            }
            args[arg_count] = NULL;

            execvp(args[0], args);
            // If we get here, exec failed
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(127);
        }
    }

    // Parent process - close all pipe fds
    for (i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (i = 0; i < num_cmds; i++) {
        waitpid(pids[i], &status, 0);
    }

    // Clean up
    for (i = 0; i < num_cmds; i++) {
        free(cmds[i]);
    }

    return OK;
}


int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    command_list_t cmd_list;
    int rc;

    while(1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        rc = build_cmd_list(cmd_line, &cmd_list);
        if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        }
        if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }
        if (rc != OK) {
            continue;
        }

        // Check for built-in commands
        if (cmd_list.num > 0) {
            Built_In_Cmds bi_cmd = match_command(cmd_list.commands[0].argv[0]);
            if (bi_cmd == BI_CMD_EXIT) {
                break;
            }
            if (bi_cmd != BI_NOT_BI) {
                rc = exec_built_in_cmd(&cmd_list.commands[0]);
                if (rc == OK_EXIT) {
                    break;
                }
                continue;
            }
        }

        rc = execute_pipeline(&cmd_list);
        if (rc != OK) {
            printf("Command execution failed\n");
        }

        free_cmd_list(&cmd_list);
    }

    return EXIT_SUCCESS;
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "rc") == 0) {
        return BI_CMD_RC;
    }
    return BI_NOT_BI;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    switch(match_command(cmd->argv[0])) {
        case BI_CMD_EXIT:
            return OK_EXIT;
            
        case BI_CMD_CD:
            if (cmd->argc == 1) {
                return OK;
            }
            if (chdir(cmd->argv[1]) != 0) {
                perror("cd failed");
                cmd->last_return_code = errno;
                return ERR_EXEC_CMD;
            }
            cmd->last_return_code = 0;
            return OK;
            
        case BI_CMD_RC:
            printf("%d\n", cmd->last_return_code);
            return OK;
            
        default:
            return BI_NOT_BI;
    }
}

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    cmd_buff->argc = 0;
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    cmd_buff->_cmd_buffer[0] = '\0';
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        cmd_buff->_cmd_buffer[0] = '\0';
    }
    cmd_buff->argc = 0;
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *token;
    char *saveptr1, *saveptr2;
    char *cmd_copy;
    char *pipe_token;
    
    // Initialize command list
    clist->num = 0;
    memset(clist->commands, 0, sizeof(clist->commands));

    // Make a copy since strtok modifies the string
    cmd_copy = strdup(cmd_line);
    if (!cmd_copy) {
        return ERR_MEMORY;
    }

    // Split by pipe character
    pipe_token = strtok_r(cmd_copy, "|", &saveptr1);
    while (pipe_token && clist->num < CMD_MAX) {
        // Allocate command buffer
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK) {
            free(cmd_copy);
            for (int i = 0; i < clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }

        // Trim whitespace
        while (*pipe_token && isspace((unsigned char)*pipe_token)) pipe_token++;
        if (strlen(pipe_token) == 0) {
            pipe_token = strtok_r(NULL, "|", &saveptr1);
            continue;
        }

        // Parse command and arguments
        token = strtok_r(pipe_token, " \t", &saveptr2);
        int arg_count = 0;
        while (token && arg_count < CMD_ARGV_MAX - 1) {
            clist->commands[clist->num].argv[arg_count++] = strdup(token);
            token = strtok_r(NULL, " \t", &saveptr2);
        }
        clist->commands[clist->num].argv[arg_count] = NULL;
        clist->commands[clist->num].argc = arg_count;

        clist->num++;
        pipe_token = strtok_r(NULL, "|", &saveptr1);
    }

    free(cmd_copy);

    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }

    if (pipe_token) {
        // More commands than CMD_MAX
        for (int i = 0; i < clist->num; i++) {
            free_cmd_buff(&clist->commands[i]);
        }
        return ERR_TOO_MANY_COMMANDS;
    }

    return OK;
}

int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    cmd_lst->num = 0;
    return OK;
}

int execute_pipeline(command_list_t *clist) {
    int pipes[CMD_MAX-1][2];
    pid_t pids[CMD_MAX];
    int pids_st[CMD_MAX];
    int exit_code;

    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    // Create processes for each command
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) { // Child process
            // Set up pipe input (except for first command)
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Set up pipe output (except for last command)
            if (i < clist->num - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe ends
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            fprintf(stderr, "%s: command not found\n", clist->commands[i].argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }

    // Get exit code from last process
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    return exit_code;
}