################################################################################
# 用于创建DLL的Makefile
#
# 说明:
# 	__declspec(dllexport): 用于在Windows中从DLL导出函数或变量的指令。
#	-shared: 用于生成DLL文件的gcc选项。
#	-L../lib: 指定链接时要查找库的目录。
#	-I../lib: 指定编译时要查找头文件的目录。
#
################################################################################
CC = gcc
CFLAGS = -Wall -Werror -O2 -shared -g
LDFLAGS = -lpthread -lws2_32 -liphlpapi
TARGET_LIB = ejt_rtk_lib.dll

# Source files
LIB_SRC = ejt_rtk_lib.c
LIB_OBJ = ejt_rtk_lib.o

.PHONY: all clean

# Default target
all: $(TARGET_LIB)

# DLL target
$(TARGET_LIB): $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object files
$(LIB_OBJ): $(LIB_SRC)
	$(CC) -c $< -o $@

# Clean
clean:
	rm -f $(LIB_OBJ) $(TARGET_LIB)
