#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
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
    if(len == 0)
         return;
    while(len > 0 && isspace((unsigned char)str[len-1])) {
         str[len-1] = '\0';
         len--;
    }
}


static void parse_single_command(char *cmd_str, command_t *cmd) {
    char *space_pos;
    
    trim_whitespace(cmd_str);
    
    space_pos = strchr(cmd_str, ' ');
    
    if (space_pos == NULL) {
        strncpy(cmd->exe, cmd_str, EXE_MAX);
        cmd->exe[EXE_MAX - 1] = '\0';
        cmd->args[0] = '\0';
    } else {
        *space_pos = '\0';
        strncpy(cmd->exe, cmd_str, EXE_MAX);
        cmd->exe[EXE_MAX - 1] = '\0';
        strncpy(cmd->args, space_pos + 1, ARG_MAX);
        cmd->args[ARG_MAX - 1] = '\0';
        trim_whitespace(cmd->args);
    }
}

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    char *cmd_copy;
    char *cmd_part;
    char *saveptr = NULL;
    int cmd_count = 0;
    
    memset(clist, 0, sizeof(command_list_t));
    
    trim_whitespace(cmd_line);
    if (strlen(cmd_line) == 0) {
        return WARN_NO_CMDS;
    }
    
    cmd_copy = strdup(cmd_line);
    if (!cmd_copy) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }
    
    cmd_part = strtok_r(cmd_copy, "|", &saveptr);
    
    while (cmd_part != NULL) {
        if (cmd_count >= CMD_MAX) {
            free(cmd_copy);
            return ERR_TOO_MANY_COMMANDS;
        }
        
        trim_whitespace(cmd_part);
        
        if (strlen(cmd_part) == 0) {
            cmd_part = strtok_r(NULL, "|", &saveptr);
            continue;
        }
        
        parse_single_command(cmd_part, &clist->commands[cmd_count]);
        
        if (strlen(clist->commands[cmd_count].exe) >= EXE_MAX) {
            free(cmd_copy);
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        
        cmd_count++;
        cmd_part = strtok_r(NULL, "|", &saveptr);
    }
    
    clist->num = cmd_count;
    free(cmd_copy);
    
    return OK;
}