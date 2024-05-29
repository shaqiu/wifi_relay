################################################################################
# This Makefile for creating the DLL
#
# 설명:
# 	__declspec(dllexport):	Windows에서 함수나 변수를 DLL에서 내보낼 때 사용하는 지시자.
#	-shared: 		DLL 파일을 생성하기 위한 gcc 옵션.
#	-L../lib: 		링크 시 라이브러리를 찾을 디렉토리를 지정.
#	-I../lib:		컴파일 시 헤더 파일을 찾을 디렉토리를 지정.
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
