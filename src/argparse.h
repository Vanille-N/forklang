#ifndef ARGPARSE_H
#define ARGPARSE_H

typedef unsigned int uint;

#define BITFLAG_UNIQUE(e) e = 1 << __COUNTER__

typedef enum {
    BITFLAG_UNIQUE(SHOW_AST),
    BITFLAG_UNIQUE(SHOW_REPR),
    BITFLAG_UNIQUE(SHOW_DOT),
    BITFLAG_UNIQUE(EXEC_RAND),
    BITFLAG_UNIQUE(EXEC_ALL),
    BITFLAG_UNIQUE(SHOW_TRACE),
    BITFLAG_UNIQUE(NO_COLOR),
    BITFLAG_UNIQUE(HELP),
} Option;

typedef struct {
    char* fname_src;
    uint flags;
} Args;

void show_help ();

Args* parse_args (int argc, char** argv);

#undef BITFLAG_UNIQUE
#endif // ARGPARSE_H
