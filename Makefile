CXX ?= g++
CXXFLAGS ?= -Wall -g
LEXER_DIR ?= ./lexer
PARSER_DIR ?= ./parser
PREPROCESSOR_DIR ?= ./basm-script
EXAMPLES_DIR ?= ./examples
LDFLAGS ?=  -lfl -ly -L$(LEXER_DIR) -L$(PARSER_DIR) -L$(PREPROCESSOR_DIR) -lparser -llexer -lbscp


LEXER_LIB = $(LEXER_DIR)/liblexer.a
PARSER_LIB = $(PARSER_DIR)/libparser.a
PREPROCESSOR_LIB = $(PREPROCESSOR_DIR)/libbscp.a
LIBS = $(LEXER_LIB) $(PARSER_LIB) $(PREPROCESSOR_LIB)

# 默认目标
all: main

# 构建词法分析器库
$(LEXER_LIB):
	$(MAKE) -C $(LEXER_DIR) LEXER_DIR=$(abspath $(LEXER_DIR))

# 构建语法分析器库
$(PARSER_LIB): $(PREPROCESSOR_DIR)/bscp.hpp
	$(MAKE) -C $(PARSER_DIR) LEXER_DIR=$(abspath $(LEXER_DIR)) PREPROCESSOR_DIR=$(abspath $(PREPROCESSOR_DIR))

# 构建预编译器库
headers: $(PREPROCESSOR_DIR)/bscp.hpp
libs: $(LIBS)
$(PREPROCESSOR_LIB) $(PREPROCESSOR_DIR)/bscp.hpp: $(LEXER_LIB)
	$(MAKE) -C $(PREPROCESSOR_DIR) LEXER_DIR=$(abspath $(LEXER_DIR))

# 编译C++主程序
main.o: main.cpp $(LEXER_DIR)/lex.h $(PARSER_DIR)/parser.h $(PREPROCESSOR_DIR)/bscp.hpp $(LIBS)
	$(CXX) $(CXXFLAGS) -I$(LEXER_DIR) -I$(PARSER_DIR) -I$(PREPROCESSOR_DIR) -c $< -o $@

# 链接程序
main: main.o $(LIBS)
	$(CXX) $(CXXFLAGS) -o $@ main.o $(LDFLAGS)

# 创建示例目录
$(EXAMPLES_DIR):
	mkdir -p $(EXAMPLES_DIR)

# 创建一个简单的测试文件
test.c:
	echo 'int main() { printf("Hello, world!\\n"); return 0; }' > test.c

# 运行测试
test: main test.c
	./main --both test.c

# 清理生成的文件
clean:
	$(MAKE) -C $(LEXER_DIR) clean
	$(MAKE) -C $(PARSER_DIR) clean
	$(MAKE) -C $(PREPROCESSOR_DIR) clean
	rm -f main.o main test.c

.PHONY: all test clean libs headers
