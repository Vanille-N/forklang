#ifndef PRINTER_H
#define PRINTER_H

#include <stdio.h>

#include "ast.h"
#include "repr.h"

typedef unsigned uint;

// Pretty-print parsed ast
// (i.e. "Niveau 1")
void pp_ast (FILE* f, bool color, prog_t* prog);

// Pretty-print internal representation
void pp_repr (FILE* f, bool color, rprog_t* prog);

// Dot-readable format 
void pp_dot (FILE* fdest, rprog_t* prog); // dump
void make_dot (char* fname_src, rprog_t* prog); // render

#endif // PRINTER_H
