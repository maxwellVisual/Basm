#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

#include "lex.h"
#include "preprocessor.h"

AstNode::~AstNode(){
    for (AstNode* node : child) {
        delete node;
    }
}

struct registered_syntax {
    parser_node_t id;
    std::string name;
    std::string match_expression;

    registered_syntax(parser_node_t id, std::string name): id(id), name(name){}
    virtual ~registered_syntax(){};
};

static std::vector<registered_syntax> registered_node_types;
static std::map<std::string, parser_node_t> type_name_table;

#ifdef __cplusplus
extern "C" {
#endif

parser_node_t parser_register_node_type(const char* name){
    try{
        return type_name_table[std::string(name)];
    }catch(std::out_of_range const&){
        registered_syntax &type = registered_node_types
            .emplace_back(registered_node_types.size(), std::string(name));
        type_name_table[type.name] = type.id;
        return type.id;
    }
}

parser_node_t parser_get_node_type(const char* name){
    try{
        return type_name_table[std::string(name)];
    }catch(std::out_of_range const&){
        return -1;
    }
}
const char* parser_get_node_type_name(parser_node_t type){
    if(registered_node_types.size() <= type){// type not found
        return nullptr;
    }
    return registered_node_types[type].name.c_str();
}


// AstNode* create_node(parser_node_t type, const char* value){
//     return new AstNode(type, value);
// }
// void free_ast(AstNode* node){
//     delete node;
// }

int parse_init(const char* input, size_t length){
    return !lex_init_with_string(input, length);
}
void parse_cleanup(void){
    lex_cleanup();
}

AstNode* ast_root;

void print_ast(AstNode* node, int level){}

void yyerror(const char* s){
    perror(s);
}

#define require_true(expr, ...) \
    do{ \
        if(!(expr)){ \
            fprintf(stderr, __VA_ARGS__); \
            return nullptr; \
        } \
    }while(false)
#define next_token require_true(lex_next(&token), "unexpected token: %s\n", token.raw)


#define CONST_NUM_TYPE_NAME "__const_num"
#define CONST_NUM_TYPE_ID parser_register_node_type(CONST_NUM_TYPE_NAME)

inline AstNode* s_const_num(AstNode* parent, lex_token& token){
    require_true(token.type == lex_number, "expecting a numeric constant: %s\n", token.raw);
    return new AstNode(CONST_NUM_TYPE_ID, parent, token.raw);
}

#define CONST_STR_TYPE_NAME "__const_str"
#define CONST_STR_TYPE_ID parser_register_node_type(CONST_STR_TYPE_NAME)


inline AstNode* s_const_str(AstNode* parent, lex_token& token){
    require_true(token.type == lex_string, "expecting a string constant: %s\n", token.raw);
    return new AstNode(CONST_STR_TYPE_ID, parent, token.raw);
}

#define EXPR_TYPE_NAME "__expr"
#define EXPR_TYPE_ID parser_register_node_type(EXPR_TYPE_NAME)

AstNode* s_expr(AstNode* parent, lex_token& token){
    switch(token.type)
    {
    case lex_number:
        return s_const_num(parent, token);
    case lex_string:
        return s_const_str(parent, token);
    default:
        require_true(false, "expecting a constant numeral or a constant string");
    }
}

#define IDENTIFIER_TYPE_NAME "__id"
#define IDENTIFIER_TYPE_ID parser_register_node_type(IDENTIFIER_TYPE_NAME)

inline AstNode* s_id(AstNode* parent, lex_token& token){
    require_true(token.type == lex_word, "expecting an identifier: %s\n", token.raw);
    return new AstNode(IDENTIFIER_TYPE_ID, parent, token.raw);
}

#define FUNCTION_DEF_TYPE_NAME "__func_def"
#define FUNCTION_DEF_TYPE_ID parser_register_node_type(FUNCTION_DEF_TYPE_NAME)

AstNode* s_func_def(AstNode* parent, lex_token& token)
{
    AstNode* node = new AstNode(FUNCTION_DEF_TYPE_ID, parent);

    require_true(token.type == lex_word, "expecting a word: %s\n", token.raw);

    next_token;
    require_true(token.type == lex_punctuation && strcmp("{", token.raw) == 0, "expecting a '{': %s\n", token.raw);

    next_token;
    while(token.type != lex_punctuation || strcmp("}", token.raw) != 0){
        require_true(token.type != lex_eol, "expecting a '}': %s\n", token.raw);

        AstNode* expr = s_expr(node, token);
        if(expr == nullptr) return nullptr;

        node->child.push_back(expr);
        next_token;
    }

    
    return node;
}

#define CODE_BLOCK_TYPE_NAME "__code_block"
#define CODE_BLOCK_TYPE_ID parser_register_node_type(FUNCTION_DEF_TYPE_NAME)

AstNode* s_code_block(AstNode* parent, lex_token& token)
{
    AstNode* node = new AstNode(CODE_BLOCK_TYPE_ID, parent);

    next_token;
    while(token.type != lex_eof){
        switch(token.type)
        {
        case lex_punctuation:
            preprocess();
            break;
        case lex_word:
            s_func_def(node, token);
            break;
        case lex_eol:
            break;
        default:
            require_true(false, "unexpected token: %s\n", token.raw);
            return nullptr;
        }
        next_token;
    }
    return node;
}


int yyparse(void){
    lex_token token;
    ast_root = s_code_block(nullptr, token);
    return ast_root == nullptr ? 1 : 0;
}

#ifdef __cplusplus
}
#endif
