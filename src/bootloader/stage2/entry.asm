bits 16

extern start

jmp entry

%include "gdt.inc"
%include "a20Line.inc"

extern start
global entry

entry:
    ; save boot drive
    mov [g_BootDrive], dl

    ; setup stack
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp

    ; switch to protected mode
    cli                     ; 1 - disable interrupt
    call EnableA20          ; 2 - Enable A20 gate
    call LoadGDT            ; 3 - Load GDT

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; 5 - far jump into protected mode 
    jmp dword 08h:.pmode

.pmode:
    ; we are now in protected mode!
    [bits 32]

    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

    ; expect boot drive in dl, send it as argument to cstart function
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call start

    jmp $

g_BootDrive: db 0
