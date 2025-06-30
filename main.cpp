#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

// 首先包含C语言词法分析器和语法分析器头文件
extern "C" {
    #define YY_NO_INPUT
    #define YY_NO_UNPUT
    #include "lex.h"
}
#include "parser.h"
#include "bscp.h"

#define LEX_TOKEN_STREAM_BUFSIZE BUFSIZ

// C++版本的词法标记类型枚举
enum class CppLexTokenType {
    Identifier,    // 标识符
    Number,        // 数字
    String,        // 字符串
    Punctuation,   // 标点符号
    Preprocessor,  // 预处理指令
    Eol,           // 行尾
    Eof,           // 文件结束
    Unknown        // 未知类型
};

// 将C词法标记类型转换为C++词法标记类型
CppLexTokenType convertTokenType(enum lex_token_type cType) {
    switch (cType) {
        case lex_word:   return CppLexTokenType::Identifier;
        case lex_number:       return CppLexTokenType::Number;
        case lex_string:       return CppLexTokenType::String;
        case lex_punctuation:  return CppLexTokenType::Punctuation;
        case lex_eol:          return CppLexTokenType::Eol;
        case lex_eof:          return CppLexTokenType::Eof;
        case lex_unknown:
        default:               return CppLexTokenType::Unknown;
    }
}

// 词法标记类
class CppLexToken {
public:
    CppLexTokenType type;
    std::string raw;
    
    CppLexToken() : type(CppLexTokenType::Unknown), raw("") {}
    
    CppLexToken(CppLexTokenType type, const std::string& raw): type(type), raw(raw) {
    }
    
    virtual ~CppLexToken() noexcept {
    }
};

// 词法标记流类
class CppLexTokenStream {
private:
    bool initialized = false;
    
public:
    CppLexTokenStream() noexcept {
        // 构造函数，不需要特殊初始化
        initialized = false;
    }
    
    /**
     * 从字符串初始化词法分析器
     * @param input 输入字符串
     * @return true表示成功，false表示失败
     */
    bool init(const std::string& input) noexcept {
        // 如果已经初始化，先清理
        if (initialized) {
            lex_cleanup();
        }
        
        // 从字符串初始化词法分析器
        initialized = (lex_init_with_string(input.c_str(), input.length()) != 0);
        return initialized;
    }
    
    /**
     * 从文件初始化词法分析器
     * @param filename 文件名
     * @return true表示成功，false表示失败
     */
    bool initFromFile(const std::string& filename) noexcept {
        // 如果已经初始化，先清理
        if (initialized) {
            lex_cleanup();
        }
        
        // 读取整个文件内容
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            return false;
        }
        
        // 获取文件大小
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // 读取文件内容到字符串
        std::string content;
        content.resize(size);
        file.read(&content[0], size);
        file.close();
        
        // 使用字符串初始化
        return init(content);
    }
    
    virtual ~CppLexTokenStream() noexcept {
        // 析构函数，清理词法分析器使用的内存
        lex_cleanup();
    }

    /**
     * 获取下一个词法标记
     * @param buf 存储词法标记的缓冲区
     * @return true表示成功，false表示遇到文件结束或错误
     */
    bool next(CppLexToken& buf) noexcept {
        // 检查是否已初始化
        if (!initialized) {
            return false;
        }
        
        // 创建一个C风格的词法标记结构体并初始化
        struct lex_token cToken;
        cToken.type = lex_unknown;   // 明确初始化为枚举类型值
        cToken.raw_size = 0;
        cToken.raw = nullptr;
        
        // 调用C语言接口获取下一个标记
        int result = lex_next(&cToken);
        
        if (result) {
            // 转换类型
            buf.type = convertTokenType(cToken.type);
            
            // 复制内容
            if (cToken.raw) {
                // 使用构造函数从C字符串创建std::string
                buf.raw = std::string(cToken.raw, cToken.raw_size);
                
                // 释放C词法分析器分配的内存
                free(cToken.raw);
                cToken.raw = nullptr;
            } else {
                buf.raw.clear();
            }
            
            return true;
        }
        
        return false;
    }
};

// 实用函数：从文件读取内容
std::string readFileContent(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return "";
    }
    
    // 读取整个文件
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 解析模式枚举
enum class ParseMode {
    LexOnly,    // 仅词法分析
    ParseOnly,  // 仅语法分析
    Both        // 词法和语法分析
};

// 注意：预处理指令处理（如#include、#define、#rule等）
// 现在由语法分析器负责处理，而不是词法分析器。
// 词法分析器仅识别预处理指令作为标记并将其传递给语法分析器。

// 主程序
int main(int argc, char* argv[]) {
    // 默认模式：仅词法分析
    ParseMode mode = ParseMode::Both;
    std::string sourceCode;
    std::string filename;
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lex" || arg == "-l") {
            mode = ParseMode::LexOnly;
        } else if (arg == "--parse" || arg == "-p") {
            mode = ParseMode::ParseOnly;
        } else if (arg == "--both" || arg == "-b") {
            mode = ParseMode::Both;
        } else {
            // 假定是文件名
            filename = arg;
        }
    }
    
    // 从文件或标准输入读取源代码
    if (!filename.empty()) {
        sourceCode = readFileContent(filename);
    } else {
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        sourceCode = buffer.str();
    }
    
    if (sourceCode.empty()) {
        std::cerr << "错误：没有源代码输入" << std::endl;
        return 1;
    }
    
    // 根据模式执行相应的分析
    if (mode == ParseMode::LexOnly || mode == ParseMode::Both) {
        // 执行词法分析
        CppLexTokenStream stream;
        if (!stream.init(sourceCode)) {
            std::cerr << "初始化词法分析器失败" << std::endl;
            return 1;
        }
        
        CppLexToken token;
        int count = 0;
        
        std::cout << "===== 词法分析开始 =====" << std::endl;
        
        while (stream.next(token)) {
            count++;
            
            // 根据词法标记类型输出不同的描述
            std::string typeStr;
            switch (token.type) {
                case CppLexTokenType::Identifier:   typeStr = "标识符"; break;
                case CppLexTokenType::Number:       typeStr = "数字"; break;
                case CppLexTokenType::String:       typeStr = "字符串"; break;
                case CppLexTokenType::Punctuation:  typeStr = "标点符号"; break;
                case CppLexTokenType::Preprocessor: typeStr = "预处理指令"; break;
                case CppLexTokenType::Eol:          typeStr = "行尾"; break;
                case CppLexTokenType::Eof:          typeStr = "文件结束"; break;
                case CppLexTokenType::Unknown:      typeStr = "未知"; break;
            }
            
            std::cout << "Token " << count << ":\t类型=" << typeStr;
            
            if (token.type == CppLexTokenType::Eof) {
                std::cout << std::endl;
                break;
            } else {
                std::cout << "\t内容=\"" << token.raw << "\"" << std::endl;
            }
        }
        
        std::cout << "总共识别 " << count << " 个词法单元" << std::endl;
        std::cout << "===== 词法分析结束 =====" << std::endl;
    }
    
    if (mode == ParseMode::ParseOnly || mode == ParseMode::Both) {
        // 执行语法分析
        std::cout << "===== 语法分析开始 =====" << std::endl;
        if(parse_init(sourceCode.c_str(), sourceCode.length())){
            std::cerr<< "语法分析器初始化失败"<<std::endl;
            return 1;
        }
        if (!yyparse()) {
            std::cout << "语法分析成功!" << std::endl;
            
            // 打印语法树
            if (ast_root != NULL) {
                std::cout << "语法树：" << std::endl;
                print_ast(ast_root, 0);
            } else {
                std::cout << "警告：生成的语法树为空" << std::endl;
            }
        } else {
            std::cerr << "语法分析失败" << std::endl;
        }
        
        // 清理资源
        parse_cleanup();
        
        std::cout << "===== 语法分析结束 =====" << std::endl;
    }
    
    return 0;
}
