all:
	gcc -o test1 test1.c -std=gnu99 -s -static

hello:
	nasm -W+all -f elf64 -o 'hello_world.o' 'hello_world.asm'
	ld -o 'hello_world.out' 'hello_world.o' -static -T hello_world.ld

	# TEST FILE USES: https://cirosantilli.com/elf-hello-world#code-3