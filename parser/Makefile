CC = g++
BISON = bison
LEXER_DIR ?=../lexer
PREPROCESSOR_DIR ?=../basm-script
CFLAGS ?= -Wall -g
CFLAGS += -I$(LEXER_DIR) -I$(PREPROCESSOR_DIR)

# 目标文件
OBJS = parser.o

# 默认目标
all: libparser.a

# 编译目标文件
parser.o: parser.cpp parser.h $(PREPROCESSOR_DIR)/bscp.hpp
	$(CC) $(CFLAGS) -c $< -o $@

# 构建语法分析器库
libparser.a: $(OBJS)
	ar rcs $@ $(OBJS)

# 清理生成的文件
clean:
	rm -f parser.o libparser.a

.PHONY: all clean 