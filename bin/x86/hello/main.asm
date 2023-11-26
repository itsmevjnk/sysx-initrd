section .data
msg: db "Hello, userland World!", 0x0A
len equ $ - msg

section .text
global _start
_start:
mov eax, 2 ; write
mov ebx, 1 ; stdout
mov ecx, len
mov edx, msg
int 0x80

mov eax, 0 ; exit
mov ecx, 0 ; exit code
int 0x80
