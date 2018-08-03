#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void clean(char * s) {
    if (0 || s==NULL)
        return;
    int i = 0;
    int j= 0;
    char* buf = malloc(strlen(s)+1);
    for (i=0; i<strlen(s); i++) {
        if (s[i] == '\n')
           continue;
        if (s[i] == '\r')
            continue;
        buf[j++]= s[i];
    }
    strcpy(s, buf);
    s[j] = 0;
    free(buf);
}

int parse(char *line, struct Parsed_line *parsed_commands) {
    clean(line);

    const char pipe_delimter[2] = "|";
    char *token;

    //parse the command list
    parsed_commands->cmd_count = 0;
    token = strtok(line, pipe_delimter);
    int i = 0;
    int in_escape = 0;
    while(token != NULL ) {
        if (in_escape) {
            in_escape = 0;
            token = strtok(NULL, pipe_delimter);
            continue;
        }
        int l = strlen(token);
        char tokens[256];
        strcpy(tokens, token);
        if (l > 1 && tokens[l-1] == '\\') {
            //escape char
            in_escape = 1;
            //token[l-1]=pipe_delimter[0];
            tokens[l-1] = '|'; //pipe_delimter;
            tokens[l] = 0;
        }
        strcpy(parsed_commands->cmds[i].command_token, tokens);
        i++;
        parsed_commands->cmd_count += 1;
        token = strtok(NULL, pipe_delimter);
    }

    //parse each command
    for (i = 0; i<parsed_commands->cmd_count; i++) {
        int cmd_found = 0;
        token = strtok(parsed_commands->cmds[i].command_token, " ");
        int in_redirect_input = 0;
        int in_redirect_output = 0;
        parsed_commands->cmds[i].args_count = 0;
        parsed_commands->cmds[i].options_count = 0;

        while (token != NULL) {
            //printf( "   parse cmd:%s\n", token );
            if (token[0] == '#') {
                break;
            }
            if (token[0] == '&') { //in background
                token = strtok(NULL, " ");
                continue;
            }
            if (token[0] == '<') {
                in_redirect_input = 1;
                token = strtok(NULL, " ");
                continue;
            }
            if (token[0] == '>') {
                in_redirect_output = 1;
                parsed_commands->cmds[i].append_out = (strlen(token) == 2 && token[1] == '>');
                token = strtok(NULL, " ");
                continue;
            }
            if (in_redirect_input) {
                strcpy(parsed_commands->cmds[i].redirect_in, token);
                in_redirect_input = 0;
                token = strtok(NULL, " ");
                continue;
            }
            if (in_redirect_output) {
                strcpy(parsed_commands->cmds[i].redirect_out, token);
                in_redirect_output = 0;
                token = strtok(NULL, " ");
                continue;
            }
            if (token[0] == '-') {
                int index = parsed_commands->cmds[i].options_count;
                strcpy(parsed_commands->cmds[i].options[index], token);
                parsed_commands->cmds[i].options_count++;
                token = strtok(NULL, " ");
                continue;
            }
            if (cmd_found) {
                int index = parsed_commands->cmds[i].args_count;
                strcpy(parsed_commands->cmds[i].args[index], token);
                parsed_commands->cmds[i].args_count++;
            }
            else {
                strcpy(parsed_commands->cmds[i].cmd, token);
                cmd_found = 1;
            }
            token = strtok(NULL, " ");

        }
    }

    return 0;
}