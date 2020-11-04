%option noyywrap
%option reentrant
%option outfile="bap_lex.c"
%option header-file="bap_lex.h"

%{
#include <bitfunc/config.h>
#include "bap_parse.h"
#include "bap_parse_types.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#ifndef HAVE_STRNDUP
/* OS X before Lion doesn't have strndup. */
static inline char *strndup(const char *s, size_t n)
{
  char *ret;
  size_t len = strlen(s);

  if (n < len) len = n;
  ret = malloc(len+1);
  ret[len] = '\0';
  return (char *)memcpy(ret, s, len);
}
#endif
#include <stdbool.h>

#define MAX_STR_CONST 1024

#define yylval (yyextra->token)

#define YY_USER_ACTION { if (!yyextra->lock_loc) { yylval.line = yyextra->line; yylval.col = yyextra->col; } yyextra->col += yyleng;}
#define LOCK_LOC yyextra->lock_loc = true
#define UNLOCK_LOC yyextra->lock_loc = false

#define TOK(x) return BAP_TOK_ ## x

%}

%x BLKCOMMENT LINECOMMENT STRING CPPTOKEN

%%

 /* Keywords */
"cjmp"      {TOK(CJMP);}
"jmp"       {TOK(JMP);}
"halt"      {TOK(HALT);}
"assert"    {TOK(ASSERT);}
"special"   {TOK(SPECIAL);}
"unknown"   {TOK(UNKNOWN);}
"with"      {TOK(WITH);}
"let"       {TOK(LET);}
"in"        {TOK(IN);}
"label"     {TOK(LABEL);}
"addr"      {TOK(ADDR);}
"extract"   {TOK(EXTRACT);}
"concat"    {TOK(CONCAT);}
"if"        {TOK(IF);}
"then"      {TOK(THEN);}
"else"      {TOK(ELSE);}
"true"      {TOK(TRUE);}
"false"     {TOK(FALSE);}
"e_bit"     {TOK(EBIG);}
"e_little"  {TOK(ELITTLE);}
 /* "{"         {TOK(LCURLY);} */
 /* "}"         {TOK(RCURLY);} */
"["         {TOK(LSQUARE);}
"]"         {TOK(RSQUARE);}
"+"         {TOK(PLUS);}
"-"         {TOK(MINUS);}
"*"         {TOK(TIMES);}
"/"         {TOK(DIVIDE);}
"$/"        {TOK(SDIVIDE);}
"%"         {TOK(MOD);}
"$%"        {TOK(SMOD);}
"<<"        {TOK(LSHIFT);}
">>"        {TOK(RSHIFT);}
"$>>"       {TOK(ARSHIFT);}
"&"         {TOK(AND);}
"|"         {TOK(OR);}
"^"         {TOK(XOR);}
"="         {TOK(EQUAL);}
"=="        {TOK(EQUALEQUAL);}
"<>"        {TOK(NEQ);}
"<"         {TOK(LT);}
"<="        {TOK(LE);}
"$<"        {TOK(SLT);}
"$<="       {TOK(SLE);}
">"         {TOK(GT);}
">="        {TOK(GE);}
"$>"        {TOK(SGT);}
"$>="       {TOK(SGE);}
"~"         {TOK(NOT);}
":="        {TOK(ASSIGN);}
":"         {TOK(COLON);}
";"         {TOK(SEMI);}
"("         {TOK(LPAREN);}
")"         {TOK(RPAREN);}
","         {TOK(COMMA);}
"@"         {TOK(AT);}
"?"         {TOK(QUESTION);}
"\""        {BEGIN(STRING); LOCK_LOC;}
"/*"        {BEGIN(BLKCOMMENT); LOCK_LOC; return BAP_TOK_BLKCOMMENT_START;}
"//"        {BEGIN(LINECOMMENT); LOCK_LOC;}
"#"         {BEGIN(CPPTOKEN);}
[[:alpha:]_][[:alnum:]_]*   {yylval.ident = strdup(yytext); return BAP_TOK_ID;}
0[xX][[:xdigit:]]+          {errno = 0; yylval.i = strtoll(yytext, NULL, 16); assert(errno == 0); return BAP_TOK_INT;}
[[:digit:]]+                {errno = 0; yylval.i = strtoll(yytext, NULL, 10); assert(errno == 0); return BAP_TOK_INT;}

\n          {yyextra->col = 1;yyextra->line++;}
[[:space:]] {}

.           {yylval.ident = strdup(yytext); return BAP_TOK_UNEXPECTED;}

<BLKCOMMENT>{
"*/"        {BEGIN(INITIAL); UNLOCK_LOC; yylval.ident = strndup(yytext, yyleng-2); return BAP_TOK_BLKCOMMENT_CHUNK; }
\n          {UNLOCK_LOC; yyextra->col = 1;yyextra->line++;yylval.ident = strdup(yytext); return BAP_TOK_BLKCOMMENT_CHUNK; }
[^\n*]+     {LOCK_LOC; yymore();}
.           {LOCK_LOC; yymore();}
}

<LINECOMMENT>{
[^\n]*\n    {BEGIN(INITIAL); UNLOCK_LOC; yylval.ident = strndup(yytext, yyleng-1); return BAP_TOK_LINECOMMENT; }
}

<CPPTOKEN>{
[^\n]*\n    {BEGIN(INITIAL);}
}

<STRING>{
"\""        {BEGIN(INITIAL); UNLOCK_LOC; yylval.ident = strndup(yytext, yyleng-1); return BAP_TOK_STRING; }
"\\"        {
              switch (yytext[0] = input(yyscanner)) {
                case 'a': yytext[0] = '\a'; break;
                case 'v': yytext[0] = '\v'; break;
                case 'f': yytext[0] = '\f'; break;
                case 'n': yytext[0] = '\n'; break;
                case 't': yytext[0] = '\t'; break;
                case 'b': yytext[0] = '\b'; break;
                case 'r': yytext[0] = '\r'; break;
              }
              yymore();
            }
[^\\"]+     {yymore();}
<<EOF>>     {printf("EOF in string\n"); return BAP_TOK_UNTERMINATED_STRING;}
}

%%
