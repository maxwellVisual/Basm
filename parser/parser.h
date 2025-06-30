#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <list>

#ifdef __cplusplus
extern "C" {
#endif


typedef size_t parser_node_t;

/* 语法树节点结构 */
typedef class AstNode {
    public:
        const parser_node_t type;   /* 节点类型 */
        char const * const value;   /* 节点值（如标识符名、常量值等） */
        std::list<AstNode*> child;  /* 子节点数组 */
        struct AstNode* parent;     /* 父节点 */

        AstNode(const parser_node_t type, AstNode* parent=nullptr, const char* value=nullptr)
            :type(type), value(value), parent(parent)
        {
            if(value == nullptr){
                this->child = std::list<AstNode*>();
            }
        }
        virtual ~AstNode();
} AstNode;

/* 注册一个新的语法树节点类型
 * @param name 节点类型的名称字符串
 * @return 返回分配的节点类型ID
 */
extern parser_node_t parser_register_node_type(const char* name);

/* 根据节点类型名称获取节点类型ID
 * @param name 节点类型的名称字符串
 * @return 返回对应的节点类型ID，如果不存在返回0
 */
extern parser_node_t parser_get_node_type(const char* name);

/* 根据节点类型ID获取节点类型名称
 * @param type 节点类型ID
 * @return 返回对应的节点类型名称，如果不存在返回NULL
 */
extern const char* parser_get_node_type_name(parser_node_t type);

/* Bison解析函数 */
extern int yyparse(void);
extern void yyerror(const char* s);

// /* 语法树相关函数 */
// AstNode* create_node(parser_node_t type, const char* value);
// void free_ast(AstNode* node);

/* 语法分析器入口 */
int parse_init(const char* input, size_t length);
void parse_cleanup(void);

/* 预处理功能 */
/* 注意：预处理功能现在由语法分析器处理，而不是词法分析器 */

/* 语法树根节点，供外部访问 */
extern AstNode* ast_root;

/* 打印语法树 */
void print_ast(AstNode* node, int level);

#ifdef __cplusplus
}
#endif

#endif /* __PARSER_H__ */ 