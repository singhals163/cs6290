# Minimal Makefile for lab5

CC := cc
CFLAGS := -std=c11 -Wall -Wextra -O2
LDFLAGS :=

TARGET := sim
SRC := lab5a.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) lab5a.c
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
