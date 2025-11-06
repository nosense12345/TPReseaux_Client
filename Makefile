# Executable names
CLIENT_EXEC = client
SERVER_EXEC = server

# Compiler
CC = gcc

# Compilation flags
CFLAGS = -Wall -Wextra -std=c11

# Source files
CLIENT_SRCS = client2.c simple_ui.c
SERVER_SRCS = server2.c

# Object files
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

# Default rule builds both
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Client build
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Server build
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean object files and executables
clean:
	rm -f *.o *.d $(CLIENT_EXEC) $(SERVER_EXEC)

# Full clean
fclean: clean

# Rebuild everything
re: fclean all

.PHONY: all clean fclean re
