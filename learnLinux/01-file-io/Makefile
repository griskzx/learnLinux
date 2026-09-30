CFLAGS = -Wall -Wextra -std=c11

.PHONY: all clean

all: file_io

file_io: file_io.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f file_io sample.txt trace.txt
