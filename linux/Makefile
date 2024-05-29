CC = gcc
CFLAG = -g -O0 -Wall -lpthread

SRCS = $(wildcard *.c */*.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
NAME = $(wildcard *.c)

TARGET = $(patsubst %.c, %, $(NAME))

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAG)

%.o:%.c
	################################################################################
	# ENABLE_DEBUG 의 default 값은 1
	################################################################################
	#$(CC) -o $@ -c $< -g -D ENABLE_DEBUG
	$(CC) -o $@ -c $< -g

clean:
	rm -rf $(TARGET) $(OBJS)
