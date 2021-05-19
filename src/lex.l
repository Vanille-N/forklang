%option noyywrap
%option yylineno

DIG	[0-9]

%%

"//".*\n { }
"var"    { return DECL; }
","      { return COMMA; }
";"      { return SEQ; }
"::"     { return BRANCH; }
"->"     { return THEN; }
"if"     { return IF; }
"fi"     { return FI; }
"do"     { return DO; }
"od"     { return OD; }
"else"   { return ELSE; }
"break"  { return BREAK; }
"||"     { return OR; }
"&&"     { return AND; }
"!"      { return NOT; }
":="     { return ASSIGN; }
"proc"   { return PROC; }
"end"    { return END; }
"=="     { return EQUAL; }
"reach"  { return REACH; }
"+"      { return ADD; }
"-"      { return SUB; }
"skip"   { return SKIP; }
">"      { return GREATER; }
"<"      { return LESS; }
"("      { return OPEN; }
")"      { return CLOSE; }

{DIG}+ { yylval.digit = atoi(yytext); return INT; }

[a-z_][a-z0-9]* { yylval.ident = strdup(yytext); return IDENT; }

[ \t\n] { }

.	{ return *yytext; }

%%

