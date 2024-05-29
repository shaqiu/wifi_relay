################################################################################
# 用于创建使用DLL的可执行文件的Makefile
#
# 说明:
# 	__declspec(dllexport): 用于在Windows中将函数或变量导出到DLL中的指令。
#	-shared: 用于生成DLL文件的gcc选项。
#	-L../lib: 指定链接时要查找库的目录。
#	-I../lib: 指定编译时要查找头文件的目录。
#
################################################################################
CC = gcc
CFLAGS = -Wall -Werror -O2 -g
LDFLAGS = -L../lib -lws2_32 -lejt_rtk_lib
INCLUDES = -I../lib

# Targets
TARGET_EXE = relay_server

# Source files
MAIN_SRC = relay_server.c
MAIN_OBJ = relay_server.o

.PHONY: all clean

# Default target
all: $(TARGET_EXE)

# Executable
$(TARGET_EXE): $(MAIN_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Object files
$(MAIN_OBJ): $(MAIN_SRC)
	$(CC) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(MAIN_OBJ) $(TARGET_EXE)
