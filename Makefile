CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude

SRC = src/main.c src/math.c src/string.c src/memory.c src/screen.c src/keyboard.c
OBJ = $(SRC:.c=.o)
TARGET = typing_tutor

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all run clean
