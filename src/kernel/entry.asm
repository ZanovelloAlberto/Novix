bits 32

section .stack
align 16
stack_bottom:
    resb 0x10000
stack_top:

section .entry
extern start
extern __bss_start
extern __end
global entry

entry:
    mov eax, [esp+4]
    mov [g_BootInfoStruct], eax

    mov esp, stack_bottom

    ; clear bss (uninitialized data) & stack
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    mov eax, [g_BootInfoStruct]
    push eax
    call start

    cli
    hlt

g_BootInfoStruct: dd 0