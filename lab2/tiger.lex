%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it
  char* output = String(str+1);
  int length = strlen(str);
  if(length <= 2) return NULL;
  int i = 1,index = 0;
  for(; i<length-1;){
    if(str[i]==92){
      if(str[i+1]>47 && str[i+1]<58){
        char num[4];
        strncpy(num,str+i+1,3);
        num[3] = '\0';
        output[index] = atoi(num);
        i+=4;
        index++;
        continue;
      }
      if(str[i+1] == '\n' || str[i+1] == '\t' || str[i+1] == ' '){
        while(str[i+1] == '\n' || str[i+1] == '\t' || str[i+1] == ' '){
          i++;
        }
        i+=2;
        continue;
      }
      switch(str[i+1]){
        case 't':
          output[index] = '\t';
          break;
        case 'n':
          output[index] = '\n';
          break;
        case 'a':
          output[index] = '\a';
          break;
        case 'b':
          output[index] = '\b';
          break;
        case 'f':
          output[index] = '\f';
          break;
        case 'r':
          output[index] = '\r';
          break;
        case 'v':
          output[index] = '\v';
          break;
        case 92:
          output[index] = '\\';
          break;
        case 34:
          output[index] = '\"';
          break;
        case 39:
          output[index] = '\'';
          break;
        case '^':
          output[index] = (str[i+2]-64);
          i++;
          break;
      }
      i+=2;
      index++;
    }else{
      output[index] = str[i];
      i++;
      index++;
    }
  }
  output[index] = '\0'; 
	return output;
}

int comment(int index){
  static int level = 0;
  level += index;
  if(level < 0){
    level = 0;
    return -1;
  }
  return level;
}

%}
  /* You can add lex definitions here. */
%START COMMENT
%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>array {adjust(); return ARRAY;}
<INITIAL>if   {adjust(); return IF;}
<INITIAL>then {adjust(); return THEN;}
<INITIAL>else {adjust(); return ELSE;}
<INITIAL>while {adjust(); return WHILE;}
<INITIAL>for  {adjust(); return FOR;}
<INITIAL>to   {adjust(); return TO;}
<INITIAL>do   {adjust(); return DO;}
<INITIAL>let  {adjust(); return LET;}
<INITIAL>in   {adjust(); return IN;}
<INITIAL>end  {adjust(); return END;}
<INITIAL>of   {adjust(); return OF;}
<INITIAL>break {adjust(); return BREAK;}
<INITIAL>nil  {adjust(); return NIL;}
<INITIAL>function {adjust(); return FUNCTION;}
<INITIAL>var  {adjust(); return VAR;}
<INITIAL>type {adjust(); return TYPE;}

<INITIAL>[0-9]+ {adjust(); yylval.ival = atoi(yytext);
             return INT;}

<INITIAL>"/*" {adjust(); BEGIN COMMENT;}

<COMMENT>"/*" {adjust(); comment(1); continue;}
<COMMENT>"*/" {adjust(); int result = comment(-1);if(result<0) BEGIN INITIAL; continue;}

<COMMENT>. {adjust(); continue;}
<COMMENT>"\n" {adjust(); EM_newline(); continue;}

<INITIAL>\"([^"]|\\\")*\" {adjust(); yylval.sval = getstr(yytext); return STRING;}
<INITIAL>([a-zA-Z]+[a-zA-Z0-9_]*)|("_main") {adjust(); yylval.sval = yytext; return ID;}

<INITIAL>","  {adjust(); return COMMA;}
<INITIAL>":"  {adjust(); return COLON;}
<INITIAL>";"  {adjust(); return SEMICOLON;}
<INITIAL>"("  {adjust(); return LPAREN;}
<INITIAL>")"  {adjust(); return RPAREN;}
<INITIAL>"["  {adjust(); return LBRACK;}
<INITIAL>"]"  {adjust(); return RBRACK;}
<INITIAL>"{"  {adjust(); return LBRACE;}
<INITIAL>"}"  {adjust(); return RBRACE;}
<INITIAL>"."  {adjust(); return DOT;}
<INITIAL>"+"  {adjust(); return PLUS;}
<INITIAL>"-"  {adjust(); return MINUS;}
<INITIAL>"*"  {adjust(); return TIMES;}
<INITIAL>"/"  {adjust(); return DIVIDE;}
<INITIAL>"="  {adjust(); return EQ;}
<INITIAL>"<>" {adjust(); return NEQ;}
<INITIAL>"<"  {adjust(); return LT;}
<INITIAL>"<=" {adjust(); return LE;}
<INITIAL>">"  {adjust(); return GT;}
<INITIAL>">=" {adjust(); return GE;}
<INITIAL>"&"  {adjust(); return AND;}
<INITIAL>"|"  {adjust(); return OR;}
<INITIAL>":=" {adjust(); return ASSIGN;}

<INITIAL>"\n" {adjust(); EM_newline(); continue;}
<INITIAL>[ \t] {adjust(); continue;}
