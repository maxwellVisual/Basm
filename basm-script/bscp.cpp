#include "bscp.h"

#include <map>
#include <string>
#include <stack>
#include <typeinfo>
#include <exception>
#include <math.h>
#include <vector>

static lex_token token;

// Forward declarations
static bscp_value* s_access_num(bscp_obj* parent, bscp_value* value0, bscp_value* value1);
static bscp_value* s_access_name(bscp_obj* parent, bscp_value* value0, bscp_value* value1);
static bscp_obj* s_obj(bscp_obj* parent);

#define require_true(expr, ...) \
    do{ \
        if(!(expr)){ \
            fprintf(stderr, __VA_ARGS__); \
            errno = 1; \
            return nullptr; \
        } \
    }while(false)
#define next_token require_true(lex_next(&token), "unexpected token: %s\n", token.raw)

const char* err_msg = nullptr;

// 表示此语法规则第一个可识别token在主体的什么位置
enum class operator_type {
    prefix,// <id_token> <expr>
    suffix,// <expr> <id_token>
    circumfix,// <id_token0> <expr> <id_token1>
    binary,// <expr> <id_token> <expr>
    triary,// <expr> <id_token0> <expr> <id_token1> <expr>
};
struct operator_mapping {
    int priority;
    operator_type type;
    std::vector<std::string> punct;
    bscp_value* (*parser)(bscp_obj* parent, ...);
};

#define UNARY_OPER_HANDLER(name, type, oper) \
static bscp_value* name(bscp_obj* parent, bscp_value* _value){ \
    require_true(typeid(_value) == typeid(type), "expecting a %s value", #type); \
    return ([](bscp_obj* parent, type* value)->bscp_value* oper)(parent, static_cast<type*>(_value)); \
}

#define BINARY_OPER_HANDLER(name, type0, type1, type_o, oper) \
static bscp_value* name(bscp_obj* parent, bscp_value* value0, bscp_value* value1){ \
    require_true(typeid(value0) == typeid(type0), "expecting a %s value", #type0); \
    require_true(typeid(value1) == typeid(type1), "expecting a %s value", #type1); \
    return ([](bscp_obj* parent, type0* a, type1* b)->type_o* oper) \
        (parent, static_cast<type0*>(value0), static_cast<type1*>(value1)); \
}
#define ALGEBRAIC_OPER_HANDLER(name, oper) \
static bscp_num* name(bscp_obj* parent, bscp_value* value0, bscp_value* value1){ \
    require_true(typeid(value0) == typeid(bscp_num*), "expecting a numeric value"); \
    require_true(typeid(value1) == typeid(bscp_num*), "expecting a numeric value"); \
    long double a = static_cast<bscp_num*>(value0)->value; \
    long double b = static_cast<bscp_num*>(value1)->value; \
    return new bscp_num(parent, (oper)); \
}
static bscp_value* s_access_tmp(bscp_obj* parent, bscp_value* value){
    if(typeid(value) == typeid(bscp_num)){
        return s_access_num(parent, parent, value);
    }else{
        return s_access_name(parent, parent, value);
    }
}

UNARY_OPER_HANDLER(s_get_increment, bscp_num, {
    bscp_num* n = new bscp_num(parent,value->value);
    value->value++;
    return n;
});
UNARY_OPER_HANDLER(s_get_decrement, bscp_num, {
    bscp_num* n = new bscp_num(parent,value->value);
    value->value--;
    return n;
});

static bscp_value* s_access_num(bscp_obj* parent, bscp_value* value0, bscp_value* value1){ 
    require_true(typeid(value0) == typeid(bscp_obj), "expecting an object");
    require_true(typeid(value1) == typeid(bscp_num), "expecting an object or a numeric value");
    ssize_t index = (ssize_t) static_cast<bscp_num*>(value1)->value;
    return (*static_cast<bscp_obj*>(value0))[index].value;
}
static bscp_value* s_access_str(bscp_obj* parent, bscp_value* value0, bscp_value* value1){ 
    require_true(typeid(value0) == typeid(bscp_obj), "expecting an object");
    require_true(typeid(value1) == typeid(bscp_obj), "expecting an object or a numeric value");
    std::string name;
    bscp_obj* obj = static_cast<bscp_obj*>(value1);
    try{
        for(size_t i = 0;; i++) {
            char c = (char)(static_cast<bscp_num*>((*obj)[i].value)->value);
            if(c == '\0'){
                break;
            }
            name += c;
        }
    }catch(const std::out_of_range&){
        require_true(false, "invalid string constant");
    }

    try{
        return (*static_cast<bscp_obj*>(value0))[name].value;
    }catch(const std::out_of_range&){
        return new bscp_null(parent);
    }
}
static bscp_value* s_access_name(bscp_obj* parent, bscp_value* value0, bscp_value* value1){
    require_true(typeid(value0) == typeid(bscp_obj), "expecting an object");

    try{
        return (*static_cast<bscp_obj*>(value0))[value1->name].value;
    }catch(const std::out_of_range&){
        return new bscp_null(parent);
    }
}

static bscp_value* s_access_offset(bscp_obj* parent, bscp_value* value0, bscp_value* value1){
    if(typeid(value1) == typeid(bscp_num)){
        return s_access_num(parent, value0, value1);
    }else{
        return s_access_str(parent, value0, value1);
    }
}
static bscp_value* s_access_attr(bscp_obj* parent, bscp_value* value0, bscp_value* value1){
    if(typeid(value1) == typeid(bscp_num)){
        return s_access_num(parent, value0, value1);
    }else{
        return s_access_name(parent, value0, value1);
    }
}

UNARY_OPER_HANDLER(s_increment_get, bscp_num, {
    value->value++;
    return value;
});
UNARY_OPER_HANDLER(s_decrement_get, bscp_num, {
    value->value--;
    return value;
});

UNARY_OPER_HANDLER(s_positate, bscp_num, {
    return new bscp_num(parent, value->value);
});
UNARY_OPER_HANDLER(s_negate, bscp_num, {
    return new bscp_num(parent, -value->value);
});
static bool value2bool(bscp_value* value){
    if(typeid(value) == typeid(bscp_num)){
        return static_cast<bscp_num*>(value)->value != 0;
    }else{
        return typeid(value) != typeid(bscp_null);
    }
}

static bscp_value* s_lnot(bscp_obj* parent, bscp_value* value){
    return new bscp_num(parent, value2bool(value) ? 1.0 : 0.0);
}
UNARY_OPER_HANDLER(s_bnot, bscp_num, {
    return new bscp_num(parent, -std::truncl(value->value) - 1);
});
inline static bscp_value* s_ref(bscp_obj* parent, bscp_value* value){
    return value->parent;// 最外层对象就是他自己，所以nullptr一定是哪儿出错了
}
UNARY_OPER_HANDLER(s_dref, bscp_obj, {
    auto it = value->fields.begin();
    if(it == value->fields.end()){
        return new bscp_null(parent);
    }else{
        return it->second.value;
    }
});

ALGEBRAIC_OPER_HANDLER(s_mul, a * b);
ALGEBRAIC_OPER_HANDLER(s_div, a / b);
ALGEBRAIC_OPER_HANDLER(s_rem, std::remainderl(a, b));
ALGEBRAIC_OPER_HANDLER(s_add, a + b);
ALGEBRAIC_OPER_HANDLER(s_sub, a - b);

ALGEBRAIC_OPER_HANDLER(s_shl, ((size_t)a) << ((size_t)b));
ALGEBRAIC_OPER_HANDLER(s_shr, ((size_t)a) >> ((size_t)b));

ALGEBRAIC_OPER_HANDLER(s_lt, a < b ?1:0);
ALGEBRAIC_OPER_HANDLER(s_le, a <= b ?1:0);
ALGEBRAIC_OPER_HANDLER(s_gt, a > b ?1:0);
ALGEBRAIC_OPER_HANDLER(s_ge, a >= b ?1:0);
ALGEBRAIC_OPER_HANDLER(s_eq, a == b ?1:0);
ALGEBRAIC_OPER_HANDLER(s_ne, a != b ?1:0);

ALGEBRAIC_OPER_HANDLER(s_band, ((size_t)a) & ((size_t)b));
ALGEBRAIC_OPER_HANDLER(s_bxor, ((size_t)a) ^ ((size_t)b));
ALGEBRAIC_OPER_HANDLER(s_bor, ((size_t)a) | ((size_t)b));

BINARY_OPER_HANDLER(s_land, bscp_value, bscp_value, bscp_num, {
    return new bscp_num(parent, (value2bool(a) && value2bool(b))?1:0);
});
BINARY_OPER_HANDLER(s_lor, bscp_value, bscp_value, bscp_num, {
    return new bscp_num(parent, (value2bool(a) || value2bool(b))?1:0);
});

static bscp_value* s_mux(
    bscp_obj* parent, 
    bscp_value* cond, 
    bscp_value* v_true,
    bscp_value* v_false
){
    return value2bool(cond) ? v_true : v_false;
}

// static bscp_value* s_assign_attr(bscp_obj* parent, bscp_value* value0, bscp_value* value1){
//     return ([](bscp_obj* parent, bscp_value* a, bscp_value* b)->bscp_value* {
//         auto it = a->parent->fields.find(a->name);
//         if(it != a->parent->fields.end()){
//             delete it->second.value;
//             if(typeid(b) == typeid(bscp_null)){
//                 it->second.value = b;
//             }else{
//                 a->parent->fields.erase(it);
//             }
//         }else{
//             if(typeid(b) != typeid(bscp_null)){
//                 a->parent->fields.emplace(a->name, Field(*b));
//             }
//         }
//         return b;
//     }) (parent, static_cast<bscp_value*>(value0), static_cast<bscp_value*>(value1)); 
// }
template<typename _T=bscp_value>
static _T* assign_field(bscp_obj* obj, std::string& name, _T* value, bool is_tmp){
    auto it = obj->fields.find(name);
    if(it != obj->fields.end()){
        delete it->second.value;
        if(typeid(value) == typeid(bscp_null)){
            it->second.value = value;
        }else{
            obj->fields.erase(it);
        }
    }else{
        if(typeid(value) != typeid(bscp_null)){
            obj->fields.emplace(name, Field(*value, is_tmp));
        }
    }
    value->name = name;
    return value;
}
BINARY_OPER_HANDLER(s_assign_attr, bscp_value, bscp_value, bscp_value, {
    return assign_field(a->parent, a->name, b, false);
});
#define ASSIGN_OPER_HANDLER(_name, oper_parser) \
BINARY_OPER_HANDLER(_name, bscp_num, bscp_num, bscp_num, { \
    return assign_field<bscp_num>(a->parent, a->name, (oper_parser)(parent, a, b), a->parent == parent); \
})
ASSIGN_OPER_HANDLER(s_add_assign, s_add);
ASSIGN_OPER_HANDLER(s_sub_assign, s_sub);
ASSIGN_OPER_HANDLER(s_mul_assign, s_mul);
ASSIGN_OPER_HANDLER(s_div_assign, s_div);
ASSIGN_OPER_HANDLER(s_rem_assign, s_rem);
ASSIGN_OPER_HANDLER(s_shl_assign, s_shl);
ASSIGN_OPER_HANDLER(s_shr_assign, s_shr);
ASSIGN_OPER_HANDLER(s_band_assign, s_band);
ASSIGN_OPER_HANDLER(s_bxor_assign, s_bxor);
ASSIGN_OPER_HANDLER(s_bor_assign, s_bor);

#define OPER_HANDLER(handler) ((bscp_value* (*)(bscp_obj* parent, ...)) (handler))
static operator_mapping oper_map[] = {
    {0, operator_type::circumfix, {"{", "}"}, OPER_HANDLER(s_obj)},
    // 优先级1: 后缀运算符和成员访问 (左结合)
    {1, operator_type::prefix, {"."}, OPER_HANDLER(s_access_tmp)},   // 成员访问
    {1, operator_type::suffix, {"++"}, OPER_HANDLER(s_get_increment)},
    {1, operator_type::suffix, {"--"}, OPER_HANDLER(s_get_decrement)},
    {1, operator_type::circumfix, {"(", ")"}, OPER_HANDLER(nullptr)},  // 函数调用
    {1, operator_type::circumfix, {"[", "]"}, OPER_HANDLER(s_access_offset)},  // 数组下标
    {1, operator_type::binary, {"."}, OPER_HANDLER(s_access_attr)},   // 成员访问
    // {1, operator_type::binary, {"->"}, OPER_HANDLER(nullptr)},  // 指针成员访问

    // 优先级2: 单目前缀运算符 (右结合)
    {2, operator_type::prefix, {"++"}, OPER_HANDLER(s_increment_get)},
    {2, operator_type::prefix, {"--"}, OPER_HANDLER(s_decrement_get)},
    {2, operator_type::prefix, {"+"}, OPER_HANDLER(s_positate)},   // 正号
    {2, operator_type::prefix, {"-"}, OPER_HANDLER(s_negate)},   // 负号
    {2, operator_type::prefix, {"!"}, OPER_HANDLER(s_lnot)},   // 逻辑NOT
    {2, operator_type::prefix, {"~"}, OPER_HANDLER(s_bnot)},   // 位NOT
    {2, operator_type::prefix, {"&"}, OPER_HANDLER(s_ref)},   // 取地址
    {2, operator_type::prefix, {"*"}, OPER_HANDLER(s_dref)},   // 解引用
    // {2, operator_type::prefix, {"(", "type", ")"}, OPER_HANDLER(nullptr)}, // 类型转换

    // 优先级3: 乘法类 (左结合)
    {3, operator_type::binary, {"*"}, OPER_HANDLER(s_mul)},
    {3, operator_type::binary, {"/"}, OPER_HANDLER(s_div)},
    {3, operator_type::binary, {"%"}, OPER_HANDLER(s_rem)},

    // 优先级4: 加法类 (左结合)
    {4, operator_type::binary, {"+"}, OPER_HANDLER(s_add)},
    {4, operator_type::binary, {"-"}, OPER_HANDLER(s_sub)},

    // 优先级5: 位移 (左结合)
    {5, operator_type::binary, {"<<"}, OPER_HANDLER(s_shl)},
    {5, operator_type::binary, {">>"}, OPER_HANDLER(s_shr)},

    // 优先级6-7: 比较 (左结合)
    {6, operator_type::binary, {"<"}, OPER_HANDLER(s_lt)},
    {6, operator_type::binary, {"<="}, OPER_HANDLER(s_le)},
    {6, operator_type::binary, {">"}, OPER_HANDLER(s_gt)},
    {6, operator_type::binary, {">="}, OPER_HANDLER(s_ge)},
    {7, operator_type::binary, {"=="}, OPER_HANDLER(s_eq)},
    {7, operator_type::binary, {"!="}, OPER_HANDLER(s_ne)},

    // 优先级8-10: 位运算 (左结合)
    {8, operator_type::binary, {"&"}, OPER_HANDLER(s_band)},
    {9, operator_type::binary, {"^"}, OPER_HANDLER(s_bxor)},
    {10, operator_type::binary, {"|"}, OPER_HANDLER(s_bor)},

    // 优先级11-12: 逻辑运算 (左结合)
    {11, operator_type::binary, {"&&"}, OPER_HANDLER(s_land)},
    {12, operator_type::binary, {"||"}, OPER_HANDLER(s_lor)},

    // 优先级13: 三元运算符 (右结合)
    {13, operator_type::triary, {"?", ":"}, OPER_HANDLER(s_mux)},

    // 优先级14: 赋值类 (右结合)
    {14, operator_type::binary, {"="}, OPER_HANDLER(s_assign_attr)},
    {14, operator_type::binary, {"+="}, OPER_HANDLER(s_add_assign)},
    {14, operator_type::binary, {"-="}, OPER_HANDLER(s_sub_assign)},
    {14, operator_type::binary, {"*="}, OPER_HANDLER(s_mul_assign)},
    {14, operator_type::binary, {"/="}, OPER_HANDLER(s_div_assign)},
    {14, operator_type::binary, {"%="}, OPER_HANDLER(s_rem_assign)},
    {14, operator_type::binary, {"<<="}, OPER_HANDLER(s_shl_assign)},
    {14, operator_type::binary, {">>="}, OPER_HANDLER(s_shr_assign)},
    {14, operator_type::binary, {"&="}, OPER_HANDLER(s_band_assign)},
    {14, operator_type::binary, {"^="}, OPER_HANDLER(s_bxor_assign)},
    {14, operator_type::binary, {"|="}, OPER_HANDLER(s_bor_assign)},

    // 优先级15: 逗号 (左结合)
    {15, operator_type::binary, {","}, OPER_HANDLER(nullptr)},
    {15, operator_type::suffix, {","}, OPER_HANDLER(nullptr)},
    {15, operator_type::binary, {";"}, OPER_HANDLER(nullptr)},
    {15, operator_type::suffix, {";"}, OPER_HANDLER(nullptr)},
    // {0, operator_type::binary, {"#"}, OPER_HANDLER(nullptr)}
};

bscp_value* s_assign_tmp(bscp_obj* parent){
    require_true(token.type == lex_punctuation && strcmp(".", token.raw) == 0, "expecting a '.': %s\n", token.raw);

    next_token;
    require_true(token.type == lex_word, "expecting an identifier: %s\n", token.raw);

    std::string name(token.raw);
    next_token;
    require_true(token.type == lex_punctuation && strcmp("=", token.raw) == 0, "expecting a '=': %s\n", token.raw);

    next_token;
    bscp_value* value = s_expr(parent);
    if(value == nullptr) return value;

    auto it = parent->fields.find(name);
    if(it != parent->fields.end()){
        delete it->second.value;
        it->second.value = value;
    }else{
        parent->fields.emplace(name, Field(*value, false));
    }
    return value;
}
/**
 * return:
 * - 0: OK
 * - !=0: failed
 */
static bscp_obj* s_obj(bscp_obj* parent){
    require_true(token.type == lex_punctuation && strcmp("{", token.raw) == 0, "expecting a '{'");
    bscp_obj* obj = new bscp_obj(parent);

    next_token;
    while(true){
        if(token.type == lex_punctuation && strcmp("}", token.raw) == 0) break;
        if(token.type == lex_eof){
            delete obj;
            return nullptr;
        }
        if(token.type == lex_punctuation && strcmp(".", token.raw) == 0) {
            s_assign_tmp(parent);
            next_token;
            continue;
        }
        if(s_expr(obj) == nullptr){
            errno = 0;
            if(err_msg != nullptr){
                fprintf(stderr, err_msg, "%s: %s", err_msg, token.raw);
            }
            delete obj;
            return nullptr;
        }
        next_token;
    }

    for(auto it = obj->fields.begin(); it != obj->fields.end();)
    {
        if(it->second.is_tmp) obj->fields.erase(it++);
        else it++;
    }

    return obj;
}

bscp_value* s_variable(bscp_obj* parent){
    require_true(token.type == lex_word, "expecting an identifier");
    std::string name(token.raw);
    bscp_value* value = parent->fields[name].value;
    value->name = name;
    return value;
}
bscp_value* s_const_num(bscp_obj* parent){
    require_true(token.type == lex_number, "expecting a numeric constant");
    return new bscp_num(parent, std::stold(token.raw));
}
bscp_value* s_const_str(bscp_obj* parent){
    require_true(token.type == lex_string, "expecting a string constant");
    bscp_obj* value = new bscp_obj(parent);
    std::string name;
    for(size_t i = 0; i<token.raw_size; i++){
        name = std::to_string(i);
        value->fields.emplace(name, Field(*new bscp_num(value, std::stold(token.raw)), false));
    }
    return value;
}
/**
 * @author deepseek-v3
 */
bscp_value* s_expr(bscp_obj* parent){
    // // assign tmp
    // if(token.type == lex_punctuation && strcmp(".", token.raw) == 0){
    //     s_assign_tmp(parent);
    //     return nullptr;
    // }

    struct value_t {
        bool is_operator;
        operator_mapping* oper;
        bscp_value* value;

        value_t(operator_mapping& oper): value(nullptr), oper(&oper), is_operator(true){}
        value_t(bscp_value& val): value(&val), oper(nullptr), is_operator(false){}
    };

    std::stack<value_t> output;
    std::stack<operator_mapping*> operators;
    std::stack<operator_mapping*> circumfix_stack; // For tracking circumfix operators

    auto apply_operator = [&output, parent](operator_mapping* op) {
        if(op->type == operator_type::prefix || op->type == operator_type::suffix) {
            require_true(output.size() >= 1, "Not enough operands for unary operator");
            auto val = output.top(); output.pop();
            output.push(value_t(*op->parser(parent, val)));
        } else if(op->type == operator_type::binary) {
            require_true(output.size() >= 2, "Not enough operands for binary operator");
            auto rhs = output.top(); output.pop();
            auto lhs = output.top(); output.pop();
            output.push(value_t(*op->parser(parent, lhs, rhs)));
        } else if(op->type == operator_type::triary) {
            require_true(output.size() >= 3, "Not enough operands for ternary operator");
            auto cond = output.top(); output.pop();
            auto if_false = output.top(); output.pop();
            auto if_true = output.top(); output.pop();
            output.push(value_t(*op->parser(parent, cond,if_true,if_false)));
        return nullptr;
        }
    };

    while(true) {
        if(token.type == lex_word || token.type == lex_number || token.type == lex_string) {
            // Handle operands
            bscp_value* val;
            if(token.type == lex_word){
                val = s_variable(parent);
            }else if(token.type == lex_number){
                val = s_const_num(parent);
            }else if(token.type == lex_string){
                val = s_const_str(parent);
            }
            output.push(value_t(*val));
            next_token;
        } else if(token.type == lex_punctuation) {
            // Handle operators
            bool found = false;
            for(auto& op : oper_map) {
                if(!op.punct.empty() && strcmp(op.punct[0].c_str(), token.raw) == 0) {
                    found = true;
                    
                    if(op.type == operator_type::circumfix) {
                        // Handle circumfix start
                        circumfix_stack.push(&op);
                        operators.push(&op);
                        next_token;
                        break;
                    } else if(!circumfix_stack.empty() && 
                              !circumfix_stack.top()->punct.empty() && 
                              strcmp(circumfix_stack.top()->punct.back().c_str(), token.raw) == 0) {
                        // Handle circumfix end
                        circumfix_stack.pop();
                        while(!operators.empty() && operators.top() != &op) {
                            apply_operator(operators.top());
                            operators.pop();
                        }
                        next_token;
                        break;
                    } else {
                        // Normal operator precedence handling
                        while(!operators.empty() && 
                              ((op.type == operator_type::prefix && op.priority < operators.top()->priority) ||
                               (op.type != operator_type::prefix && op.priority <= operators.top()->priority))) {
                            apply_operator(operators.top());
                            operators.pop();
                        }
                        operators.push(&op);
                        next_token;
                        break;
                    }
                }
            }
            require_true(found, "Unknown operator: %s", token.raw);
        } else if(token.type == lex_punctuation && strcmp("(", token.raw) == 0) {
            // Handle parentheses
            next_token;
            bscp_value* subexpr = s_expr(parent);
            require_true(subexpr != nullptr, "Invalid subexpression");
            output.push(value_t(*subexpr));
            require_true(token.type == lex_punctuation && strcmp(")", token.raw) == 0, 
                        "Missing closing parenthesis");
            next_token;
        } else {
            // End of expression
            break;
        }
    }

    // Apply remaining operators
    while(!operators.empty()) {
        apply_operator(operators.top());
        operators.pop();
    }

    require_true(!output.empty(), "Empty expression");
    require_true(output.size() == 1, "Malformed expression");
    require_true(!output.top().is_operator, "Dangling operator");

    return output.top().value;
}
