# Minimal Makefile for lab5

CC := cc
CFLAGS := -std=c11 -Wall -Wextra -O2
LDFLAGS :=

TARGET := sim
SRC := main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) systolic.c fifo.c pe.c
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
