#ifndef PRINTER_H
#define PRINTER_H

#include <stdio.h>

#include "ast.h"
#include "repr.h"
#include "exec.h"

typedef unsigned uint;

// Pretty-print parsed ast
// (i.e. "Niveau 1")
void pp_ast (FILE* f, bool color, Prog* prog);

// Pretty-print internal representation
void pp_repr (FILE* f, bool color, RProg* prog);

// Dot-readable format 
void pp_dot (FILE* fdest, RProg* prog); // dump
void make_dot (char* fname_src, RProg* prog); // render

// Reachability trace
void pp_sat (RProg* prog, Sat* sat, bool color, bool trace, bool exhaustive);

#endif // PRINTER_H
