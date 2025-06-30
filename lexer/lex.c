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

/* 动态缓冲区管理 - 设为全局变量以便lex.lex可以访问 */
char* dynamic_buffer = NULL;    /* 动态缓冲区指针 */
size_t buffer_size = 0;         /* 当前缓冲区大小 */
size_t buffer_capacity = 0;     /* 当前缓冲区容量 */

/* 字符串缓冲区管理 - 设为全局变量以便lex.lex可以访问 */
char* string_buffer = NULL;         /* 字符串缓冲区 */
size_t string_buffer_size = 0;      /* 字符串缓冲区大小 */
size_t string_buffer_capacity = 0;  /* 字符串缓冲区容量 */

/* 初始化动态缓冲区 */
void init_dynamic_buffer(void) {
    if (dynamic_buffer == NULL) {
        buffer_capacity = INITIAL_BUFFER_SIZE;
        dynamic_buffer = (char*)malloc(buffer_capacity);
        if (!dynamic_buffer) {
            fprintf(stderr, "内存分配失败\n");
            exit(EXIT_FAILURE);
        }
    }
    buffer_size = 0;
}

/* 向动态缓冲区添加内容 */
void append_to_buffer(const char* text, size_t len) {
    /* 检查是否需要扩展缓冲区 */
    if (buffer_size + len + 1 > buffer_capacity) {
        size_t new_capacity = buffer_capacity * BUFFER_GROWTH_FACTOR;
        while (buffer_size + len + 1 > new_capacity) {
            new_capacity *= BUFFER_GROWTH_FACTOR;
        }
        
        char* new_buffer = (char*)realloc(dynamic_buffer, new_capacity);
        if (!new_buffer) {
            fprintf(stderr, "内存重分配失败\n");
            exit(EXIT_FAILURE);
        }
        
        dynamic_buffer = new_buffer;
        buffer_capacity = new_capacity;
    }
    
    /* 复制内容到缓冲区 */
    memcpy(dynamic_buffer + buffer_size, text, len);
    buffer_size += len;
    dynamic_buffer[buffer_size] = '\0'; /* 确保以NULL结尾 */
}

/* 释放动态缓冲区 */
void free_dynamic_buffer(void) {
    if (dynamic_buffer) {
        free(dynamic_buffer);
        dynamic_buffer = NULL;
        buffer_size = 0;
        buffer_capacity = 0;
    }
}

/* 初始化字符串缓冲区 */
void init_string_buffer(void) {
    free_string_buffer(); /* 先清理可能存在的旧缓冲区 */
    string_buffer_capacity = INITIAL_BUFFER_SIZE;
    string_buffer = (char*)malloc(string_buffer_capacity);
    if (!string_buffer) {
        fprintf(stderr, "字符串缓冲区分配失败\n");
        exit(EXIT_FAILURE);
    }
    string_buffer_size = 0;
}

/* 向字符串缓冲区添加内容 */
void append_to_string_buffer(const char* text, size_t len) {
    /* 检查是否需要扩展缓冲区 */
    if (string_buffer_size + len + 1 > string_buffer_capacity) {
        size_t new_capacity = string_buffer_capacity * BUFFER_GROWTH_FACTOR;
        while (string_buffer_size + len + 1 > new_capacity) {
            new_capacity *= BUFFER_GROWTH_FACTOR;
        }
        
        char* new_buffer = (char*)realloc(string_buffer, new_capacity);
        if (!new_buffer) {
            fprintf(stderr, "字符串缓冲区重分配失败\n");
            exit(EXIT_FAILURE);
        }
        
        string_buffer = new_buffer;
        string_buffer_capacity = new_capacity;
    }
    
    /* 复制内容到缓冲区 */
    memcpy(string_buffer + string_buffer_size, text, len);
    string_buffer_size += len;
    string_buffer[string_buffer_size] = '\0'; /* 确保以NULL结尾 */
}

/* 释放字符串缓冲区 */
void free_string_buffer(void) {
    if (string_buffer) {
        free(string_buffer);
        string_buffer = NULL;
        string_buffer_size = 0;
        string_buffer_capacity = 0;
    }
}

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
    free_dynamic_buffer();
    free_string_buffer();
    if (current_token.raw) {
        free(current_token.raw);
        current_token.raw = NULL;
    }
} 