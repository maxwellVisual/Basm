CXX ?= g++
LEXER_DIR ?=../lexer
CXXFLAGS ?= -Wall -g
CXXFLAGS += -I$(LEXER_DIR)
BISON ?= bison
YACCFLAGS += -Lc++
# 目标文件
OBJS = bscp.tab.o bscp.o 

# 默认目标
all: libbscp.a bscp
bscp.tab.cpp bscp.hpp: bscp.ypp
	$(BISON) $(YACCFLAGS) $< -o bscp.tab.cpp -Hbscp.hpp

# 编译目标文件
%.o: %.cpp $(LEXER_DIR)/liblexer.a bscp.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# 构建预处理器库
libbscp.a: $(OBJS)
	ar rcs $@ $(OBJS)

bscp: $(OBJS) $(LEXER_DIR)/liblexer.a
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -L$(LEXER_DIR) -llexer

# 清理生成的文件
clean:
	rm -f $(OBJS) bscp.hpp bscp.tab.cpp libbscp.a stack.hh

.PHONY: all clean
