#include "lex.h"

#ifdef __cplusplus
#include <map>
#include <memory>
#include <stdexcept>

class bscp_obj;

class bscp_value
{
public:
    std::string name;
    bscp_obj* parent;
protected:
    bscp_value(bscp_obj* parent): parent(parent){}
};

class bscp_null: public bscp_value
{
public:
    bscp_null(bscp_obj* parent): bscp_value(parent){}
};

class bscp_num: public bscp_value
{
public:
    long double value;

    // positive value only
    bscp_num(bscp_obj* parent, std::string& raw)
        : bscp_value(parent), value(std::strtold(raw.c_str(), nullptr)){}

    bscp_num(bscp_obj* parent, long double value)
        : bscp_value(parent), value(value){}

    bscp_num(bscp_obj* parent)
        : bscp_value(parent), value(0){}
};

class Field
{
public:
    std::string name;
    bscp_value* value;
    bool is_tmp;
    Field(): value(nullptr), is_tmp(false) {}
    Field(bscp_value& value, bool is_tmp = true): value(&value), is_tmp(is_tmp){}
};
class bscp_obj: public bscp_value
{
public:
    std::map<std::string, Field> fields;
    bscp_obj(bscp_obj* parent): bscp_value(parent){}
    inline Field& operator[](size_t id){
        return this->fields[std::to_string(id)];
    }
    inline Field& operator[](std::string& name){
        return this->fields[name];
    }
};

extern "C" {
#endif


extern bscp_value* s_expr(bscp_obj* parent);

extern int preprocess();

#ifdef __cplusplus
}
#endif
