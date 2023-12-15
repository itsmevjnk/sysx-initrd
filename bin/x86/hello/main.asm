section .data
msg: db "Hello, userland World!", 0x0A
len equ $ - msg
msg_child: db "(from another, forked, process)", 0x0A
len_child equ $ - msg_child

section .bss
child_done: dd 0

section .text
global _start
_start:
mov eax, 2 ; write
mov ebx, 1 ; stdout
mov ecx, len
mov edx, msg
int 0x80

mov eax, 3 ; fork
int 0x80
test eax, eax
jnz .parent

.child:
mov eax, 2 ; write
mov ebx, 1 ; stdout
mov ecx, len_child
mov edx, msg_child
int 0x80
inc dword [child_done]
jmp .exit ; forked process also needs to exit too!

.parent: ; wait for child task to finish its job
mov eax, [child_done]
test eax, eax
jz .parent

.exit:
mov eax, 0 ; exit
mov ecx, 0 ; exit code
int 0x80
