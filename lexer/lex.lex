%{
#include "lex.h"

/* 声明会使用到的外部变量和函数 */
extern char* yytext;
extern int yyleng;

/* 设置词法标记并返回 */
/* 设置token类型并复制yytext内容 */
#define SET_TOKEN(token_type) \
    do { \
        current_token.type = token_type; \
        current_token.raw_size = yyleng; \
        current_token.raw = yyleng > 0 ? strndup(yytext, yyleng) : NULL; \
        return 1; \
    } while(0)

/* 设置不同类型token的辅助宏 */
#define WORD_TOKEN()            SET_TOKEN(lex_word)
#define NUMBER_TOKEN()          SET_TOKEN(lex_number)
#define STRING_TOKEN()          SET_TOKEN(lex_string) 
#define PUNCTUATION_TOKEN()     SET_TOKEN(lex_punctuation)
#define EOL_TOKEN()             SET_TOKEN(lex_eol)
#define ASSEMBLY_TOKEN()        SET_TOKEN(lex_assembly)
#define UNKNOWN_TOKEN()         SET_TOKEN(lex_unknown)

/* EOF的特殊处理 */
#define EOF_TOKEN() \
    do { \
        current_token.type = lex_eof; \
        current_token.raw = NULL; \
        current_token.raw_size = 0; \
        return 1; \
    } while(0)
%}

/* Flex选项 */
%option noyywrap
%option never-interactive
%option noinput
%option nounput

/* 定义词法分析器状态 */
%x COMMENT
/* %x STRING
%x CHAR */
/* %x PREPROC */

/* 词法模式定义 */
DIGIT       [0-9]
LETTER      [a-zA-Z_]
ID          {LETTER}({LETTER}|{DIGIT})*
WHITESPACE  [ \t\r\f]
NEWLINE     [\n]
INTEGER     ([1-9]{DIGIT}*)|0
FLOAT       {INTEGER}\.{DIGIT}+([eE][+-]?{DIGIT}+)?
SCIENTIFIC  {DIGIT}+[eE][+-]?{DIGIT}+
HEX         0[xX][0-9a-fA-F]+
OCTAL       0[1-7][0-7]*
BINARY      0[bB][01]+
FMT_CHAR    (\\\')|(\\\")|(\\\?)|(\\\\)|(\\a)|(\\b)|(\\f)|(\\n)|(\\r)|(\\t)|(\\v)|(\\x[0-9a-fA-F]+)|(\\u[0-9a-fA-F]{4})|(\\U[0-9a-fA-F]{8})|(\\[0-7]{1,3})

%%
 /* ======= 规则部分 ======= */
 /* ^#  { PUNCTUATION_TOKEN(); BEGIN(PREPROC);  } */
 /* 这个好像不会和比较运算冲突，因为比较运算中间一定有逻辑运算符 */
\<[a-zA-Z0-9/\.]+\>  { STRING_TOKEN(); }

 /* 处理换行符作为EOL标记 */
{NEWLINE}   {
    EOL_TOKEN(); 
    // BEGIN(INITIAL); 
}

 /* 标识符 */
{ID}        { 
    WORD_TOKEN(); 
}

 /* 数字常量 - 支持各种格式 */
{INTEGER}       { NUMBER_TOKEN(); }
{FLOAT}         { NUMBER_TOKEN(); }
{SCIENTIFIC}    { NUMBER_TOKEN(); }
{HEX}           { NUMBER_TOKEN(); }
{OCTAL}         { NUMBER_TOKEN(); }
{BINARY}        { NUMBER_TOKEN(); }

 /* 字符串常量处理 */
\"([^\\"]|{FMT_CHAR})*\" { STRING_TOKEN(); }

 /* 字符常量处理 */
\'([^\\"]|{FMT_CHAR})\'  { NUMBER_TOKEN(); }


 /* 注释处理 */
"/*"                { BEGIN(COMMENT); }
<COMMENT>[^*]*      { /* 忽略注释内容 */ }
<COMMENT>"*"+[^*/]* { /* 忽略注释内容 */ }
<COMMENT>"*"+"/"    { BEGIN(INITIAL); }

"//"[^\n]*          { /* 忽略单行注释 */ }

 /* 
  * 运算符和标点符号 
  * 为提高可读性，按照类别分组
  */
 /* 算术运算符 */
"+"         { PUNCTUATION_TOKEN(); }
"-"         { PUNCTUATION_TOKEN(); }
"*"         { PUNCTUATION_TOKEN(); }
"/"         { PUNCTUATION_TOKEN(); }
"%"         { PUNCTUATION_TOKEN(); }
"++"        { PUNCTUATION_TOKEN(); }
"--"        { PUNCTUATION_TOKEN(); }

 /* 赋值运算符 */
"="         { PUNCTUATION_TOKEN(); }
"+="        { PUNCTUATION_TOKEN(); }
"-="        { PUNCTUATION_TOKEN(); }
"*="        { PUNCTUATION_TOKEN(); }
"/="        { PUNCTUATION_TOKEN(); }
"%="        { PUNCTUATION_TOKEN(); }
"&="        { PUNCTUATION_TOKEN(); }
"|="        { PUNCTUATION_TOKEN(); }
"^="        { PUNCTUATION_TOKEN(); }
"<<="       { PUNCTUATION_TOKEN(); }
">>="       { PUNCTUATION_TOKEN(); }

 /* 比较运算符 */
"=="        { PUNCTUATION_TOKEN(); }
"!="        { PUNCTUATION_TOKEN(); }
">"         { PUNCTUATION_TOKEN(); }
"<"         { PUNCTUATION_TOKEN(); }
">="        { PUNCTUATION_TOKEN(); }
"<="        { PUNCTUATION_TOKEN(); }

 /* 逻辑运算符 */
"&&"        { PUNCTUATION_TOKEN(); }
"||"        { PUNCTUATION_TOKEN(); }
"!"         { PUNCTUATION_TOKEN(); }

 /* 位运算符 */
"&"         { PUNCTUATION_TOKEN(); }
"|"         { PUNCTUATION_TOKEN(); }
"^"         { PUNCTUATION_TOKEN(); }
"~"         { PUNCTUATION_TOKEN(); }
"<<"        { PUNCTUATION_TOKEN(); }
">>"        { PUNCTUATION_TOKEN(); }

 /* 特殊符号和分隔符 */
"->"        { PUNCTUATION_TOKEN(); }
"."         { PUNCTUATION_TOKEN(); }
","         { PUNCTUATION_TOKEN(); }
";"         { PUNCTUATION_TOKEN(); }
":"         { PUNCTUATION_TOKEN(); }
"?"         { PUNCTUATION_TOKEN(); }
"("         { PUNCTUATION_TOKEN(); }
")"         { PUNCTUATION_TOKEN(); }
"["         { PUNCTUATION_TOKEN(); }
"]"         { PUNCTUATION_TOKEN(); }
"{"         { PUNCTUATION_TOKEN(); }
"}"         { PUNCTUATION_TOKEN(); }

 /* 不在行首的#号当作普通标点符号处理 */
"#"         { PUNCTUATION_TOKEN(); }

 /* 空白符 - 全部忽略，但换行符除外（已经单独处理） */
{WHITESPACE}+ { /* 忽略空白符 */ }

 /* 处理未知字符 */
.           { UNKNOWN_TOKEN(); }

 /* 处理文件结束 */
<<EOF>>     { EOF_TOKEN(); }

%%
