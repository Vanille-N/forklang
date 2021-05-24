#include "argparse.h"
#include "prelude.h"

typedef struct {
    char* long_name;
    char short_name;
    Option option;
    char* help_message;
} Flag;

Flag opt_flags [] = {
    { "ast", 'a', SHOW_AST, "Pretty-print the syntax tree" },
    { "repr", 'r', SHOW_REPR, "Pretty-print the internal graph representation" },
    { "dot", 'd', SHOW_DOT, "Dump graphviz file and render as png" },
    { "rand", 'R', EXEC_RAND, "Perform Monte-Carlo execution" },
    { "all", 'A', EXEC_ALL, "Perform exhaustive execution" },
    { "trace", 't', SHOW_TRACE, "Show sequence of steps to satisfy checks" },
    { "no-color", 'c', NO_COLOR, "Do not use ANSI color codes in pretty-prints" },
    { "help", 'h', HELP, "Show help message and exit" },
    { NULL, 0, 0, NULL },
};

void show_help () {
    printf("lang\n");
    printf("  Parser, pretty-printer and simulator\n");
    printf("\n");
    printf("  Usage: lang [FILE] [FLAGS]\n");
    printf("  Flags:\n");
    for (uint j = 0; opt_flags[j].long_name; j++) {
        printf("    -%c, --%-10s   %50s\n",
            opt_flags[j].short_name,
            opt_flags[j].long_name,
            opt_flags[j].help_message);
    }
    printf("  Examples:\n");
    printf("      lang -ar input.prog --no-color\n");
    printf("      lang input.prog --rand --all -c -t\n");
    printf("      lang -h");
}

uint multiflags (char* c) {
    uint acc = 0;
    for (; *c; c++) {
        uint j;
        for (j = 0; opt_flags[j].long_name; j++) {
            if (*c == opt_flags[j].short_name) {
                if (acc & opt_flags[j].option) {
                    fprintf(stderr, "Warning: duplicate flag '%c' is ignored\n", *c);
                }
                acc |= opt_flags[j].option;
                break;
            }
        }
        if (!opt_flags[j].long_name) {
            fprintf(stderr, "Unknown flag '%c'\n", *c);
            return 0;
        }
    }
    return acc;
}

uint find_option (char* arg) {
    if (arg[1] == '-') { // caller must check arg[0] == '-'
        for (uint j = 0; opt_flags[j].long_name; j++) {
            if (0 == strcmp(arg+2, opt_flags[j].long_name)) {
                return opt_flags[j].option;
            }
        }
        return 0;
    }
    return multiflags(arg+1);
}

Args* parse_args (int argc, char** argv) {
    Args* args = malloc(sizeof(Args));
    args->fname_src = NULL;
    args->flags = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            uint opt = find_option(argv[i]);
            if (opt) {
                if (args->flags & opt) {
                    fprintf(stderr,
                        "Warning: duplicate flag '%s' is ignored\n",
                        argv[i]);
                }
                args->flags |= opt;
            } else {
                fprintf(stderr, "No such option '%s'\n", argv[i]);
                show_help();
                free(args);
                return NULL;
            }
        } else {
            if (args->fname_src) {
                fprintf(stderr,
                    "Warning: duplicate filename '%s' is ignored\n",
                    argv[i]);
            } else {
                args->fname_src = argv[i];
            }
        }
    }
    if ((args->flags&SHOW_TRACE)
        && !(args->flags&EXEC_RAND)
        && !(args->flags&EXEC_ALL)) {
            fprintf(stderr,
                "Warning: --trace is useless without either --rand or --all\n");
    }
    if (args->flags&HELP) {
        show_help();
        free(args);
        return NULL;
    }
    if (!args->fname_src) {
        fprintf(stderr, "No file specified\n");
        show_help();
        free(args);
        return NULL;
    }
    return args;
}
