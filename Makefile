CC = gcc
CFLAGS = -std=c17 -pedantic-errors -Wall -Wextra -O2
TARGET = main
SRCS = main.c lexer.c parser.c generator.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
$(OBJS): %.o: %.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<
clean:
	-rm -f $(TARGET) $(OBJS)

.PHONY: all clean
