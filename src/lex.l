%option noyywrap
%option yylineno
%option nounput
%option noinput

%{

#include "memreg.h"
MemBlock* ident_alloc_registry = NULL;
void register_ident (void* ptr) { register_alloc(&ident_alloc_registry, ptr); }
void free_ident () { register_free(&ident_alloc_registry); }

%}

DIG	[0-9]

%%

"//"[^\n]* { }
"var"      { return DECL; }
","        { return COMMA; }
";"        { return SEQ; }
"::"       { return BRANCH; }
"->"       { return THEN; }
"if"       { return IF; }
"fi"       { return FI; }
"do"       { return DO; }
"od"       { return OD; }
"else"     { return ELSE; }
"break"    { return BREAK; }
"||"       { return OR; }
"&&"       { return AND; }
"!"        { return NOT; }
":="       { return ASSIGN; }
"proc"     { return PROC; }
"end"      { return END; }
"=="       { return EQ; }
"reach"    { return REACH; }
"+"        { return ADD; }
"-"        { return SUB; }
"*"        { return MUL; }
"skip"     { return SKIP; }
">"        { return GT; }
"<"        { return LT; }
"<="       { return LEQ; }
">="       { return GEQ; }
"%"        { return MOD; }
"/"        { return DIV; }
"("        { return OPEN; }
")"        { return CLOSE; }
"\.\."     { return RANGE; }

{DIG}+ { yylval.digit = (uint)atoi(yytext); return INT; }

[a-z_][a-z0-9]* { register_ident(yylval.ident = strdup(yytext)); return IDENT; }

[ \t\n] { }

.	{ return *yytext; }

%%

