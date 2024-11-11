;
; File: main.s
;
; Description:
;      Contains the kernel's entry point function and initializes the stack.
;
; Author: Novice
; last modification: 11/9th/2024
;


bits 32

section .text

global _start
extern kmain

_start:
    mov esp, stack_space
    call kmain
    jmp $

section .bss
    resb 8192
stack_space:

