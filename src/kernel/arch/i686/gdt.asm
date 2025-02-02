global i686_GDT_flush
i686_GDT_flush:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, [ebp + 8]
    lgdt [eax]

    ; reload code segment
    jmp 0x08:.reload_cs

.reload_cs:
    ; reload data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret