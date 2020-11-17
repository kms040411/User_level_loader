CC = gcc
CFLAGS = -std=gnu99 -static -Wl,-Ttext-segment=0x20000000

TARGET = test1 hello_world.out segfault test2 test3

all:	hello
	$(CC) -o test1 test1.c $(CFLAGS)
	$(CC) -o test2 test2.c $(CFLAGS)
	$(CC) -o test3 test3.c $(CFLAGS)
	$(CC) -o segfault segfault.c $(CFLAGS)

hello:
	# ORIGINAL TEST FILE : https://cirosantilli.com/elf-hello-world#code-3
	nasm -W+all -f elf64 -o 'hello_world.o' 'hello_world.asm'
	ld -o 'hello_world.out' 'hello_world.o' -static -T hello_world.ld

clean:
	rm $(TARGET)