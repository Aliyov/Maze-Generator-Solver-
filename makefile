# Compiler
CC = gcc

# Source files
SRC = maze.c

# Output executable
TARGET = maze

# Compilation and linking flags
CFLAGS = -Wall -Wextra -Werror `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean up build files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean

