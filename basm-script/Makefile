CC ?= g++
BISON ?= bison
LEXER_DIR ?=../lexer
CFLAGS ?= -Wall -g -I$(LEXER_DIR)

# 目标文件
OBJS = bscp.o

# 默认目标
all: libbscp.a

# 编译目标文件
bscp.o: bscp.cpp bscp.h $(LEXER_DIR)/liblexer.a
	$(CC) $(CFLAGS) -c $< -o $@

# 构建预处理器库
libbscp.a: $(OBJS)
	ar rcs $@ $(OBJS)

# 清理生成的文件
clean:
	rm -f libbscp.a $(OBJS)

.PHONY: all clean 