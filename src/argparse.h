#ifndef ARGPARSE_H
#define ARGPARSE_H

typedef unsigned int uint;

typedef enum {
    SHOW_AST = 1, SHOW_REPR = 2, SHOW_DOT = 4,
    EXEC_RAND = 8, EXEC_ALL = 16,
    SHOW_TRACE = 32,
    NO_COLOR = 64,
    HELP = 128,
} Option;

typedef struct {
    char* fname_src;
    uint flags;
} Args;

void show_help ();

Args* parse_args (int argc, char** argv);

#endif // ARGPARSE_H
