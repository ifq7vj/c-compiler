TARGET = main
SRCS = main.c lexer.c parser.c generator.c
OBJS = $(SRCS:.c=.o)

CC = gcc
CFLAGS = -std=c17 -pedantic-errors -Wall -Wextra -O2

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJS): %.o: %.c main.h
	$(CC) $(CFLAGS) -c -o $@ $<
