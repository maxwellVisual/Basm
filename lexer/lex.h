#ifndef __LEX_H__
#define __LEX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 公共常量定义 */
#define LEX_TOKEN_STREAM_BUFSIZE BUFSIZ
#define INITIAL_BUFFER_SIZE 64
#define BUFFER_GROWTH_FACTOR 2

/* 词法标记类型枚举 */
enum lex_token_type {
    lex_word, // id, name, etc.
    lex_number,     // 1, 2, 3, etc.
    lex_string,     // "hello", "world", etc.
    lex_punctuation,// ;, +, ), etc.
    lex_eol,        // end of line
    lex_eof,        // end of file
    lex_assembly,   // assmebly block
    lex_unknown,    // unknown token
};

/* 词法标记结构 */
struct lex_token {
    enum lex_token_type type;
    size_t raw_size;
    char* raw;
};

/* 全局变量声明 */
extern struct lex_token current_token;


/* 公共接口函数 */
/**
 * 使用字符串作为输入初始化词法分析器
 * 
 * @param input 输入字符串
 * @param length 字符串长度，如果为0则使用strlen计算
 * @return 1表示成功，0表示失败
 */
extern int lex_init_with_string(const char* input, size_t length);

/**
 * 获取下一个词法单元
 *
 * @param buf 用于存储词法单元的缓冲区
 * @return 1表示成功，0表示遇到错误或文件结束
 */
extern int lex_next(struct lex_token* buf);

/**
 * 清理词法分析器使用的所有动态分配内存
 * 应该在词法分析完成后调用此函数
 */
extern void lex_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif
