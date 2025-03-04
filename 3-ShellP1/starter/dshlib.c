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
    char redirect_type = '\0';
    bool is_append = false;

    clear_cmd_buff(cmd_buff);
    
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    current = cmd_buff->_cmd_buffer;
    
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_output = false;
    
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
        } else if (((*current == '<') || (*current == '>')) && !in_quotes) {
            // Handle redirection operators
            if (token_start != current) {
                *current = '\0';
                cmd_buff->argv[argc++] = token_start;
            }
            
            // Save the redirection type
            redirect_type = *current;
            is_append = false;
            
            // Check for append operator (>>)
            if (redirect_type == '>' && *(current + 1) == '>') {
                is_append = true;
                current++; // Skip the second '>'
            }
            
            // Null-terminate and move past the redirection operator
            *current = '\0';
            current++;
            
            // Skip any whitespace after the operator
            while (*current && isspace((unsigned char)*current)) {
                current++;
            }
            
            if (!*current) {
                return ERR_CMD_ARGS_BAD; // Missing filename after redirection operator
            }
            
            // Mark the start of the filename
            token_start = current;
            
            // Find the end of the filename (next whitespace or another operator)
            while (*current && !isspace((unsigned char)*current) && *current != '<' && *current != '>') {
                current++;
            }
            
            // Save the current character and null-terminate the filename
            char saved = *current;
            if (*current) {
                *current = '\0';
            }
            
            // Store the filename in the appropriate field
            if (redirect_type == '<') {
                cmd_buff->input_file = token_start;
            } else if (redirect_type == '>') {
                cmd_buff->output_file = token_start;
                cmd_buff->append_output = is_append;
            }
            
            // If we're at the end of the string, we're done
            if (!saved) {
                break;
            }
            
            // Otherwise, move to the next character and continue
            token_start = current + 1;
            
            // Skip any whitespace
            while (*(current + 1) && isspace((unsigned char)*(current + 1))) {
                current++;
            }
        } else if (isspace((unsigned char)*current) && !in_quotes) {
            if (token_start != current) {
                *current = '\0';
                cmd_buff->argv[argc++] = token_start;
            }
            
            while (*(current + 1) && isspace((unsigned char)*(current + 1))) {
                current++;
            }
            token_start = current + 1;
        }
        
        current++;
    }
    
    // Add the last token if there is one and it's not a redirection filename
    if (token_start && *token_start && argc < CMD_ARGV_MAX - 1) {
        if ((cmd_buff->input_file == NULL || cmd_buff->input_file != token_start) && 
            (cmd_buff->output_file == NULL || cmd_buff->output_file != token_start)) {
            cmd_buff->argv[argc++] = token_start;
        }
    }
    
    // Null-terminate the argv array
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
    int input_fd = -1;
    int output_fd = -1;

    pid = fork();
    if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (cmd->input_file != NULL) {
            input_fd = open(cmd->input_file, O_RDONLY);
            if (input_fd == -1) {
                fprintf(stderr, "Error opening input file %s: %s\n", cmd->input_file, strerror(errno));
                exit(errno);
            }
            
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                perror("dup2 input failed");
                exit(EXIT_FAILURE);
            }
            
            close(input_fd);
        }
        
        // Handle output redirection
        if (cmd->output_file != NULL) {
            // Use O_APPEND flag if append_output is true
            int flags = O_WRONLY | O_CREAT;
            if (cmd->append_output) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            
            output_fd = open(cmd->output_file, flags, 0644);
            if (output_fd == -1) {
                fprintf(stderr, "Error opening output file %s: %s\n", cmd->output_file, strerror(errno));
                exit(errno);
            }
            
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("dup2 output failed");
                exit(EXIT_FAILURE);
            }
            
            close(output_fd);
        }
        
        // Execute the command
        execvp(cmd->argv[0], cmd->argv);
        
        // If execvp returns, there was an error
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
    command_list_t cmd_list;
    char cmd_line[SH_CMD_MAX];
    int rc;
    
    // Initialize command list
    memset(&cmd_list, 0, sizeof(command_list_t));

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
        
        // Build command list from input line
        rc = build_cmd_list(cmd_line, &cmd_list);
        
        if (rc != OK) {
            if (rc == WARN_NO_CMDS) {
                printf("%s", CMD_WARN_NO_CMD);
            } else if (rc == ERR_TOO_MANY_COMMANDS) {
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            }
            free_cmd_list(&cmd_list);
            continue;
        }
        
        // Check if the first command is a built-in command
        Built_In_Cmds bi_cmd = match_command(cmd_list.commands[0].argv[0]);
        
        if (bi_cmd != BI_NOT_BI && cmd_list.num == 1) {
            // Execute built-in command (only for single commands, not in pipes)
            rc = exec_built_in_cmd(&cmd_list.commands[0]);
            if (rc == OK_EXIT) {
                free_cmd_list(&cmd_list);
                break;
            }
        } else {
            // Execute pipeline of commands
            rc = execute_pipeline(&cmd_list);
            if (rc != OK) {
                printf("Command execution failed\n");
            }
        }
        
        // Free command list resources
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
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_output = false;
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *token;
    char *saveptr;
    int cmd_count = 0;
    char cmd_line_copy[SH_CMD_MAX];
    
    // Initialize command list
    clist->num = 0;
    
    // Make a copy of cmd_line since strtok_r modifies the string
    strncpy(cmd_line_copy, cmd_line, SH_CMD_MAX - 1);
    cmd_line_copy[SH_CMD_MAX - 1] = '\0';
    
    // Tokenize by pipe character
    token = strtok_r(cmd_line_copy, PIPE_STRING, &saveptr);
    
    while (token != NULL && cmd_count < CMD_MAX) {
        // Allocate memory for the command buffer
        if (alloc_cmd_buff(&clist->commands[cmd_count]) != OK) {
            // Clean up previously allocated buffers
            for (int i = 0; i < cmd_count; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }
        
        // Trim whitespace from the token
        trim_whitespace(token);
        
        // Build command buffer from the token
        int rc = build_cmd_buff(token, &clist->commands[cmd_count]);
        if (rc != OK && rc != WARN_NO_CMDS) {
            // Clean up allocated buffers
            for (int i = 0; i <= cmd_count; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return rc;
        }
        
        // Only increment command count if we have a valid command
        if (rc == OK) {
            cmd_count++;
        }
        
        // Get next token
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    
    // Check if we have too many commands
    if (token != NULL && cmd_count >= CMD_MAX) {
        // Clean up allocated buffers
        for (int i = 0; i < cmd_count; i++) {
            free_cmd_buff(&clist->commands[i]);
        }
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Update the number of commands
    clist->num = cmd_count;
    
    // Return appropriate status
    if (cmd_count == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

int execute_pipeline(command_list_t *clist) {
    int i;
    int status;
    int pipe_fds[2];
    int prev_pipe_read = -1;
    pid_t pids[CMD_MAX];
    int input_fd = -1;
    int output_fd = -1;
    
    // Check if we have any commands
    if (clist->num <= 0) {
        return WARN_NO_CMDS;
    }
    
    // Execute each command in the pipeline
    for (i = 0; i < clist->num; i++) {
        // Create a pipe for all but the last command
        if (i < clist->num - 1) {
            if (pipe(pipe_fds) == -1) {
                perror("pipe failed");
                // Clean up previous pipe if it exists
                if (prev_pipe_read != -1) {
                    close(prev_pipe_read);
                }
                return ERR_EXEC_CMD;
            }
        }
        
        // Fork a child process
        pids[i] = fork();
        
        if (pids[i] == -1) {
            // Fork failed
            perror("fork failed");
            
            // Clean up pipes
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            if (i < clist->num - 1) {
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }
            
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Handle input redirection for the first command
            if (i == 0 && clist->commands[i].input_file != NULL) {
                input_fd = open(clist->commands[i].input_file, O_RDONLY);
                if (input_fd == -1) {
                    fprintf(stderr, "Error opening input file %s: %s\n", 
                            clist->commands[i].input_file, strerror(errno));
                    exit(errno);
                }
                
                if (dup2(input_fd, STDIN_FILENO) == -1) {
                    perror("dup2 input failed");
                    exit(EXIT_FAILURE);
                }
                
                close(input_fd);
            } else if (prev_pipe_read != -1) {
                // Connect input to previous pipe if not the first command
                if (dup2(prev_pipe_read, STDIN_FILENO) == -1) {
                    perror("dup2 input failed");
                    exit(EXIT_FAILURE);
                }
                close(prev_pipe_read);
            }
            
            // Handle output redirection for the last command
            if (i == clist->num - 1 && clist->commands[i].output_file != NULL) {
                // Use O_APPEND flag if append_output is true
                int flags = O_WRONLY | O_CREAT;
                if (clist->commands[i].append_output) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                
                output_fd = open(clist->commands[i].output_file, flags, 0644);
                if (output_fd == -1) {
                    fprintf(stderr, "Error opening output file %s: %s\n", 
                            clist->commands[i].output_file, strerror(errno));
                    exit(errno);
                }
                
                if (dup2(output_fd, STDOUT_FILENO) == -1) {
                    perror("dup2 output failed");
                    exit(EXIT_FAILURE);
                }
                
                close(output_fd);
            } else if (i < clist->num - 1) {
                // Connect output to pipe if not the last command
                if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
                    perror("dup2 output failed");
                    exit(EXIT_FAILURE);
                }
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, there was an error
            switch(errno) {
                case ENOENT:
                    fprintf(stderr, "%s: command not found in PATH\n", clist->commands[i].argv[0]);
                    break;
                case EACCES:
                    fprintf(stderr, "%s: permission denied\n", clist->commands[i].argv[0]);
                    break;
                case ENOEXEC:
                    fprintf(stderr, "%s: not an executable\n", clist->commands[i].argv[0]);
                    break;
                case ENOMEM:
                    fprintf(stderr, "%s: out of memory\n", clist->commands[i].argv[0]);
                    break;
                default:
                    perror(clist->commands[i].argv[0]);
            }
            exit(errno);
        } else {
            // Parent process
            
            // Close previous pipe read end if it exists
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            
            // Save the read end of the current pipe and close the write end
            if (i < clist->num - 1) {
                prev_pipe_read = pipe_fds[0];
                close(pipe_fds[1]);
            }
        }
    }
    
    // Close the last pipe read end if it exists
    if (prev_pipe_read != -1) {
        close(prev_pipe_read);
    }
    
    // Wait for all child processes to complete
    for (i = 0; i < clist->num; i++) {
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid failed");
            return ERR_EXEC_CMD;
        }
        
        // Store the return code in the last command
        if (i == clist->num - 1) {
            if (WIFEXITED(status)) {
                clist->commands[i].last_return_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                clist->commands[i].last_return_code = 128 + WTERMSIG(status);
            }
        }
    }
    
    return OK;
}

int free_cmd_list(command_list_t *cmd_list) {
    for (int i = 0; i < cmd_list->num; i++) {
        free_cmd_buff(&cmd_list->commands[i]);
    }
    cmd_list->num = 0;
    return OK;
}