################################################################################
# This Makefile for creating the executable that uses the DLL
#
# 설명:
# 	__declspec(dllexport):	Windows에서 함수나 변수를 DLL에서 내보낼 때 사용하는 지시자.
#	-shared: 		DLL 파일을 생성하기 위한 gcc 옵션.
#	-L../lib: 		링크 시 라이브러리를 찾을 디렉토리를 지정.
#	-I../lib:		컴파일 시 헤더 파일을 찾을 디렉토리를 지정.
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
