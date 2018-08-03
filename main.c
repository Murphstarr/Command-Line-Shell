#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "parse.h"
#include <fcntl.h>

//A correct executable of the parser is in ~wayne/pub/cs146/nsh-parser
//You can assume there are spaces around *every* token even though my example doesn't have a space between < and "infile"

//TODO wc does not work in pipes??
//TODO remove printf

/*BONUS
background - 1 pt
# as comment in line - 1 pt
Allow meta-characters - 1 pt, who|wc works not who | wc
escape - 1 pt  Implement the shell escape character using "\", so "echo \|" will output a single "|".
exit status - 1 pt except it does not pick up last cmd exit status
> 1 pipes - 3 pt
*/

void display_command(char *line) {
    if (line==NULL || strlen(line)==0 || line[0] == '#')
        return;

    struct Parsed_line parsed_line;
    memset(&parsed_line, 0, sizeof(parsed_line));
    int res = parse(line, &parsed_line);
    printf("%d: ", parsed_line.cmd_count);
    int i = 0;
    int j = 0;
    for (i=0; i<parsed_line.cmd_count; i++) {
        if (strlen(parsed_line.cmds[i].redirect_in) >0) {
            printf(" <'%s' ", parsed_line.cmds[i].redirect_in);
        }
        if (strlen(parsed_line.cmds[i].cmd) > 0 ) {
            printf("'%s'", parsed_line.cmds[i].cmd);
        }
        for (j=0; j<parsed_line.cmds[i].options_count; j++) {
            printf(" '%s' ", parsed_line.cmds[i].options[j]);
        }
        for (j=0; j<parsed_line.cmds[i].args_count; j++) {
            printf(" '%s' ", parsed_line.cmds[i].args[j]);
        }
        if (strlen(parsed_line.cmds[i].redirect_out) >0) {
            if (parsed_line.cmds[i].append_out == 1) {
                printf(" >>'%s' ", parsed_line.cmds[i].redirect_out);
            }
            else {
                printf(" >'%s' ", parsed_line.cmds[i].redirect_out);
            }
        }
        if (i < parsed_line.cmd_count-1) {
            printf(" | ");
        }
    }
    printf("\n");
}

int exit_status =0;

void run_command(char *line) {
    if (line==NULL || strlen(line)==0 || line[0] == '#')
        return ;

    int MAX_PROCS = 256;
    int MAX_OPT_COUNT = 256;
    int proc_id[MAX_PROCS];
    int pipefd[MAX_PROCS][2];

    struct Parsed_line parsed_line;
    memset(&parsed_line, 0, sizeof(parsed_line));
    int res = parse(line, &parsed_line);

    int in_background = 0;
    int len = strlen(line);
    if (len>0) {
        if (line[len-1]=='&') {
            in_background = 1;
        }
    }
    //printf("%d: ", parsed_line.cmd_count);

    int command_number = 0;
    for (command_number=0; command_number<parsed_line.cmd_count; command_number++) {

        if (strcmp(parsed_line.cmds[command_number].cmd, "exit") == 0) {
            if (parsed_line.cmds[command_number].args_count > 0)
                exit_status = atoi(parsed_line.cmds[command_number].args[0]);
            return;
        }
        if (command_number < parsed_line.cmd_count) {
            pipe(pipefd[command_number]);
        }

        proc_id[command_number] = fork();
        if (proc_id[command_number] == 0) {
            //printf("in child %d %s\n", command_number, parsed_line.cmds[command_number].cmd);
            if (command_number < parsed_line.cmd_count-1) {
                //pipe output
                dup2(pipefd[command_number][1], STDOUT_FILENO);
                close(pipefd[command_number][0]);
            }
            else {
                //output redirect
                if (parsed_line.cmds[command_number].redirect_out[0] != 0) {
                    //printf("in child redirect test1 %d\n", command_number);

                    int fd = 0;
                    if (parsed_line.cmds[command_number].append_out) {
                        //printf("in append_redirect\n");
                        fd = open(parsed_line.cmds[command_number].redirect_out, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
                    }
                    else {
                        //printf("in redirect\n");
                        FILE *f;
                        if ((f = fopen(parsed_line.cmds[command_number].redirect_out, "r"))) {
                            fclose(f);
                            //printf("in redirect removed \n");
                            remove(parsed_line.cmds[command_number].redirect_out);
                        }
                        fd = open(parsed_line.cmds[command_number].redirect_out, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    }
                    //printf("in child no pipe end %d\n", command_number);

                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
            }

            if (parsed_line.cmds[command_number].redirect_in[0] != 0) {
                //input redirect
                int fd = open(parsed_line.cmds[command_number].redirect_in, O_RDONLY, S_IRUSR | S_IWUSR);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            else {
                if (command_number > 0) {
                    //pipe input
                    dup2(pipefd[command_number - 1][0], STDIN_FILENO);
                    close(pipefd[command_number - 1][1]);
                }
            }

            //Run the command
            char* args[MAX_OPT_COUNT];
            memset(args, 0, sizeof(args));
            args[0] = malloc(256);
            strcpy(args[0], parsed_line.cmds[command_number].cmd);
            int i = 0;
            int argctr = 1;
            for (i=0; i<parsed_line.cmds[command_number].options_count; i++) {
                args[argctr] = malloc(256);
                strcpy(args[argctr++], parsed_line.cmds[command_number].options[i]);
            }
            for (i=0; i<parsed_line.cmds[command_number].args_count; i++) {
                args[argctr] = malloc(256);
                strcpy(args[argctr++], parsed_line.cmds[command_number].args[i]);
            }
            execvp(args[0], args);
            perror("exec");
            //When execvp() is executed, the program file given by the first argument will be loaded into the caller's address space and over-write the program there.
            //printf("end child1\n");
            //return 1;
        }
        else {
            //printf("-->created proc id %d\n", proc_id[command_number]);
        }
    }
    if (!in_background) {
        int i;
        //printf("-->parent start wait for %d childs\n", parsed_line.cmd_count);
        //for (i=0; i<parsed_line.cmd_count; i++) {
            int status;
            int last_proc_id = proc_id[parsed_line.cmd_count-1];
            //printf("-->parent start wait for %d\n", last_proc_id);
            //waitpid(last_proc_id, &status, 0);
            wait(&status);
            exit_status = status;
            //printf("-->parent end wait state %d %d\n", i, exit_status);
        //}
    }
}

int main(int argc, char **argv) {
    if (argc == 2) {
        FILE *fp = fopen(argv[1], "r");
        size_t len = 255;
        char line[256];
        if (fp == NULL) {
            printf("cant open file %s", argv[1]);
            return 0;
        }
        while(fgets(line, len, fp) != NULL) {
            clean(line);
            if (strlen(line) > 0) {
                //display_command(line);
                //printf("LINE %s\n", line);
                run_command(line);
                //sleep(1);
            }
        }
    }
    else {
        size_t bsize = 256;
        while (1) { //TODO ensure Ctrl-D sends EOF on stdin
            printf("\n? ");
            char* line_buffer = (char *) malloc (bsize);
            int bytes_read = getline (&line_buffer, &bsize, stdin);
            if (bytes_read == -1) break;
            //display_command(line_buffer);
            run_command(line_buffer);
            free (line_buffer);
        }
    }
    //printf("program exit:%d\n", exit_status);
    return exit_status;
}