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

static void trim_whitespace(char *str) {
    int start = 0, i;
    int len;

    while(str[start] && isspace((unsigned char)str[start])) {
        start++;
    }
    if(start > 0) {
        for(i = 0; str[i+start] != '\0'; i++) {
            str[i] = str[i+start];
        }
        str[i] = '\0';
    }

    len = strlen(str);
    if(len == 0) return;
    while(len > 0 && isspace((unsigned char)str[len-1])) {
        str[len-1] = '\0';
        len--;
    }
}

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
      
        execvp(cmd->argv[0], cmd->argv);
        
      
        switch(errno) {
            case ENOENT:
                fprintf(stderr, "%s: command not found in PATH\n", cmd->argv[0]);
                break;
            case EACCES:
                fprintf(stderr, "%s: permission denied\n", cmd->argv[0]);
                break;
            case ENOEXEC:
                fprintf(stderr, "%s: not an executable\n", cmd->argv[0]);
                break;
            case ENOMEM:
                fprintf(stderr, "%s: out of memory\n", cmd->argv[0]);
                break;
            default:
                perror(cmd->argv[0]);
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

int exec_local_cmd_loop() {
    cmd_buff_t cmd;
    char cmd_line[SH_CMD_MAX];
    int rc;
    if (alloc_cmd_buff(&cmd) != OK) {
        return ERR_MEMORY;
    }

    while(1) {
        printf("%s", SH_PROMPT);
        
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        trim_whitespace(cmd_line);
        if (strlen(cmd_line) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }
        rc = build_cmd_buff(cmd_line, &cmd);
        if (rc != OK) {
            if (rc == WARN_NO_CMDS) {
                printf("%s", CMD_WARN_NO_CMD);
            }
            continue;
        }
        Built_In_Cmds bi_cmd = match_command(cmd.argv[0]);
        if (bi_cmd != BI_NOT_BI) {
            rc = exec_built_in_cmd(&cmd);
            if (rc == OK_EXIT) {
                break;
            }
            continue;
        }
        rc = exec_cmd(&cmd);
        if (rc != OK) {
            printf("Command execution failed\n");
        }
    }

    free_cmd_buff(&cmd);
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