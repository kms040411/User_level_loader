all:
	gcc -o test1 test1.c -std=gnu99 -static -Wl,-Ttext-segment=0x20000000

hello:
	nasm -W+all -f elf64 -o 'hello_world.o' 'hello_world.asm'
	ld -o 'hello_world.out' 'hello_world.o' -static -T hello_world.ld -v

	# ORIGINAL TEST FILE : https://cirosantilli.com/elf-hello-world#code-3