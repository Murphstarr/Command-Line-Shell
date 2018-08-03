#define MAX_COMMANDS 32
#define MAX_ARGS 32
#define MAX_OPTS 32

struct Parsed_command {
    char command_token[256];
    char cmd[64];
    int options_count;
    char options[64][MAX_OPTS];
    int args_count;
    char args[64][MAX_ARGS];
    char redirect_in[64];
    char redirect_out[64];
    int append_out;
};

struct Parsed_line {
    int cmd_count;
    struct Parsed_command cmds[MAX_COMMANDS];
};

int parse(char* line, struct Parsed_line *parsed_commands);
void clean(char * s);
