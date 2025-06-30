#include "bscp.h"

#include <string>
#include <iostream>

#include "lex.h"
#include "parser.h"

static bscp_obj root_obj;

// 1 if c is an oct digit else 0
#define isodigit(c) ((c) >= '0' && (c) < '8')

static inline int 
format_string_u(std::string& dest, size_t& i, size_t length, char c){
    int required_digits = (c == 'u') ? 4 : 8;
    i++;
    int unicode_val = 0;
    for (int digits = 0; digits < required_digits && i < length; digits++, i++) {
        if (!isxdigit(c)) return 1;
        unicode_val = (unicode_val << 4) |  // More efficient than *16
            (isdigit(c) ? c - '0' : 
             tolower(c) - 'a' + 10);
    }

    // Handle UTF-8 encoding (unchanged logic)
    if (unicode_val <= 0x7F) {
        dest.push_back(static_cast<char>(unicode_val));
    } else if (unicode_val <= 0x7FF) {
        dest.push_back(static_cast<char>(0xC0 | (unicode_val >> 6)));
        dest.push_back(static_cast<char>(0x80 | (unicode_val & 0x3F)));
    } else if (unicode_val <= 0xFFFF) {
        dest.push_back(static_cast<char>(0xE0 | (unicode_val >> 12)));
        dest.push_back(static_cast<char>(0x80 | ((unicode_val >> 6) & 0x3F)));
        dest.push_back(static_cast<char>(0x80 | (unicode_val & 0x3F)));
    } else if (unicode_val <= 0x10FFFF) {
        dest.push_back(static_cast<char>(0xF0 | (unicode_val >> 18)));
        dest.push_back(static_cast<char>(0x80 | ((unicode_val >> 12) & 0x3F)));
        dest.push_back(static_cast<char>(0x80 | ((unicode_val >> 6) & 0x3F)));
        dest.push_back(static_cast<char>(0x80 | (unicode_val & 0x3F)));
    } else {
        return 1; // Invalid Unicode codepoint
    }
    i--; // Adjust for final loop increment
    return 0;
}
static inline int 
format_string_x(std::string& dest, size_t& i, size_t length, char c){
    // Hex escape
    i++;
    if (i >= length) return 1;
    int hex_val = 0;
    int digits = 0;
    for (; i < length && isxdigit(c); i++, digits++) {
        hex_val = (hex_val << 4) |  // More efficient than *16
            (isdigit(c) ? c - '0' : 
             tolower(c) - 'a' + 10);
    }
    
    if (digits == 0) return 1; // No hex digits
    dest.push_back(static_cast<char>(hex_val));
    i--; // Adjust for final loop increment
    return 0;
}
static inline int 
format_string_o(std::string& dest, size_t& i, size_t length, char c) {
    if (!isodigit(c)) return 1;
    
    int oct_val = 0;
    for (int digits = 0; 
         digits < 3 && i < length && isodigit(c); 
         digits++, i++) {
        oct_val = (oct_val << 3) | (c - '0');  // More efficient than *8
    }
    
    dest.push_back(static_cast<char>(oct_val));
    i--; // Adjust for final loop increment
    return 0;
}

static int format_string(const std::string& str, std::string& dest) {
    dest.clear();
    dest.reserve(str.length());

    size_t i = 0;
    const size_t len = str.length();
    const char* data = str.data();

    while (i < len) {
        // Find next escape sequence
        size_t escape_pos = str.find('\\', i);
        if (escape_pos == std::string::npos) {
            // No more escapes - append remaining characters
            dest.append(data + i, len - i);
            break;
        }

        // Append characters before escape
        if (escape_pos > i) {
            dest.append(data + i, escape_pos - i);
        }

        i = escape_pos + 1; // Skip backslash
        if (i >= len) return 1; // Error: incomplete escape

        // Process escape sequence
        switch (data[i]) {
            case '\'': case '\"': case '\?': case '\\':
                dest.push_back(data[i]);
                break;
            case 'a': dest.push_back('\a'); break;
            case 'b': dest.push_back('\b'); break;
            case 'f': dest.push_back('\f'); break;
            case 'n': dest.push_back('\n'); break;
            case 'r': dest.push_back('\r'); break;
            case 't': dest.push_back('\t'); break;
            case 'v': dest.push_back('\v'); break;
            case 'x':
                if(format_string_x(dest, i, len, data[i])) return 1;
                break;
            case 'u': case 'U': // Unicode
                if(format_string_u(dest, i, len, data[i])) return 1;
                break;
            default: // Octal escape
                if(format_string_o(dest, i, len, data[i])) return 1;
        }
        i++; // Move past processed character
    }

    return 0;
}

static lex_token token;

#define require_true(expr, ...) \
    do{ \
        if(!(expr)){ \
            fprintf(stderr, __VA_ARGS__); \
            return; \
        } \
    }while(false)
#define next_token require_true(lex_next(&token), "unexpected token: %s\n", token.raw)

/**
 * log <str>
 */
static int _log(){
    require_true(token.type == lex_word && strcmp("log", token.raw) == 0, "expecting preprocessor 'log': %s\n", token.raw);

    next_token;
    require_true(token.type == lex_string, "expecting a string: %s\n", token.raw);

    std::string msg;
    int ret = format_string(std::string(token.raw+1, token.raw_size-2), msg);
    if(ret){
        return ret;
    }
    std::cout<<msg;

    return 0;
}
struct _arg_t{
    std::string name;
    std::string type;
};
/**
 * <name> [: <type>] 
 */
static int _arg(){
    require_true(token.type == lex_word, "expecting an identifier: %s\n", token.raw);

    next_token;
    if(token.type != lex_punctuation || strcmp(":", token.raw) == 0){
        return 0;
    }
}
/**
 * (<arg>, <arg>...)
 */
static int _args(){
    require_true(token.type == lex_punctuation && strcmp("(", token.raw) == 0, "expecting a '(': %s\n", token.raw);

    next_token;
    while(true){
        require_true(token.type == lex_punctuation && strcmp(")", token.raw) == 0, "expecting a ')' or ',': %s\n", token.raw);

    }

}

static int _abstract(){
    require_true(token.type == lex_word && strcmp("abstract", token.raw) == 0, "expecting preprocessor 'abstract': %s\n", token.raw);
    
    next_token;
    require_true(token.type == lex_word, "expecting an identifier: %s\n", token.raw);
    std::string identifier(token.raw);
    
    next_token;
    _args();

    next_token;
    _block();

}
#ifdef __cplusplus
extern "C" {
#endif

static int _block(){
    next_token;

    require_true(token.type == lex_word, "expecting a word: %s", token.raw);

    if(strcmp("log", token.raw) == 0){
        _log();
    }else if(strcmp("abstract", token.raw) == 0){
        _abstract();
    }else{
        require_true(false, "invalid preprocessor: %s\n", token.raw);
    }


    next_token;
    require_true(token.type == lex_eol, "expecting an EOL: %s\n", token.raw);
}

int preprocess(){
    return _block();
    // while(token.type != lex_eof){
    //     if(token.type != lex_punctuation || strcmp("#", token.raw) != 0){
    //         next_token;
    //         continue;
    //     }
    //     next_token;
    //     block();
    // }
}

#ifdef __cplusplus
}
#endif
