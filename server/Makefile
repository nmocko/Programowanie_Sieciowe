# Name of the execution file
TARGET = server

# Compiler
CC = gcc

# Compilation flags
# CFLAGS = -Wall -Wextra -std=c11

# Header files
HEADERS = multicast.h read_write.h game.h utils.h

# Source files
SRCS = main.c multicast.c read_write.c game.c utils.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default rule: compiles all of the files
all: $(TARGET)

# Execution file compilation
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Object files compilation
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Cleaning rule
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
