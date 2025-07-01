#include "lex.h"

/* Flex相关类型定义 */
struct yy_buffer_state;
typedef struct yy_buffer_state* YY_BUFFER_STATE;

/* Flex生成的函数前向声明 */
extern int yylex(void);
extern YY_BUFFER_STATE yy_scan_bytes(const char* bytes, int len);
extern void yy_switch_to_buffer(YY_BUFFER_STATE buffer);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern char* yytext;
extern int yyleng;

/* 全局变量，用于存储当前识别的token */
struct lex_token current_token = { lex_unknown, 0, NULL };

/* 用于字符串输入的缓冲区 */
static YY_BUFFER_STATE string_buffer_state = NULL;


/**
 * 使用字符串作为输入初始化词法分析器
 * 
 * @param input 输入字符串
 * @param length 字符串长度，如果为0则使用strlen计算
 * @return 1表示成功，0表示失败
 */
int lex_init_with_string(const char* input, size_t length) {
    /* 清理可能存在的旧缓冲区 */
    if (string_buffer_state) {
        yy_delete_buffer(string_buffer_state);
        string_buffer_state = NULL;
    }
    
    /* 计算长度（如果未指定） */
    if (length == 0 && input) {
        length = strlen(input);
    }
    
    /* 检查输入有效性 */
    if (!input || length == 0) {
        return 0;
    }
    
    /* 创建新的输入缓冲区 */
    string_buffer_state = yy_scan_bytes(input, (int)length);
    if (!string_buffer_state) {
        return 0;
    }
    
    /* 设置为当前缓冲区 */
    yy_switch_to_buffer(string_buffer_state);
    
    return 1;
}

/**
 * 获取下一个词法单元
 * 
 * @param buf 用于存储词法单元的缓冲区
 * @return 1表示成功，0表示遇到错误或文件结束
 */
int lex_next(struct lex_token* buf) {
    int ret = yylex();
    if (ret) {
        /* 复制当前token到输出buffer */
        buf->type = current_token.type;
        buf->raw_size = current_token.raw_size;
        
        if (current_token.raw) {
            /* 直接传递raw指针的所有权给调用者，调用者负责释放内存 */
            buf->raw = current_token.raw;
            current_token.raw = NULL;
        } else {
            buf->raw = NULL;
        }
        
        return 1;
    }
    
    return 0;
}

/**
 * 词法分析器销毁函数 - 清理所有动态分配的内存
 */
void lex_cleanup(void) {
    /* 清理缓冲区 */
    if (string_buffer_state) {
        yy_delete_buffer(string_buffer_state);
        string_buffer_state = NULL;
    }
    
    /* 清理其他资源 */
    if (current_token.raw) {
        free(current_token.raw);
        current_token.raw = NULL;
    }
}
