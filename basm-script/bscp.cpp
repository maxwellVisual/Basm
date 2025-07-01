#include "bscp.hpp"

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>

#include "lex.h"

const char *argp_program_version = "bscp 1.0";
const char *argp_program_bug_address = "<bug@example.com>";
static char doc[] = "bscp - A simple expression language interpreter";
static char args_doc[] = "[FILE]";

static struct argp_option options[] = {
    {"debug", 'd', 0, 0, "Enable debug output"},
    {0}
};

struct arguments {
    char *file;
    int debug;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = (struct arguments *)state->input;
    
    switch (key) {
    case 'd':
        arguments->debug = 1;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num >= 1)
            argp_usage(state);
        arguments->file = arg;
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 0)
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int execute_file(const char *filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    
    yyin = file;
    int result = yy::parser()();
    fclose(file);
    
    return result;
}

int repl_mode() {
    printf("bscp REPL (press Ctrl+D to exit)\n");
    
    char line[1024];
    while (printf("> ") && fgets(line, sizeof(line), stdin)) {
        if (strlen(line) == 1 && line[0] == '\n') continue;
        
        yyin = fmemopen(line, strlen(line), "r");
        int result = yy::parser()();
        fclose(yyin);
        
        if (result != 0) {
            fprintf(stderr, "Error parsing input\n");
        }
    }
    
    printf("\n");
    return 0;
}

__attribute__((weak)) 
int main(int argc, char *argv[]) {
    struct arguments arguments;
    arguments.file = NULL;
    arguments.debug = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.file) {
        return execute_file(arguments.file);
    } else {
        return repl_mode();
    }
}
