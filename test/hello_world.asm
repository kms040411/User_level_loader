section .data
    hello_world db "Hello world!", 10
section .text
    global _start
    _start:
        mov rax, 1
        mov rdi, 1
        lea rsi, [rel $ + 0x1FFFF6]
        mov rdx, 0x0E
        syscall
        mov rax, 60
        mov rdi, 0
        syscall