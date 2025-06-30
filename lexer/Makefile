CC = gcc
FLEX = flex
CFLAGS = -Wall -g

# 目标文件
OBJS = lex.yy.o lex.o

# 默认目标
all: liblexer.a

# 生成词法分析器
lex.yy.c: lex.lex
	$(FLEX) -o $@ $<

# 编译目标文件
lex.yy.o: lex.yy.c lex.h
	$(CC) $(CFLAGS) -c $<

lex.o: lex.c lex.h
	$(CC) $(CFLAGS) -c $<

# 构建词法分析器库
liblexer.a: $(OBJS)
	ar rcs $@ $(OBJS)

# 清理生成的文件
clean:
	rm -f lex.yy.c lex.yy.o lex.o lexer_rules.o liblexer.a

.PHONY: all clean 