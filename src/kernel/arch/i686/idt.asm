global i686_IDT_flush
i686_IDT_flush:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov eax, [ebp + 8]
    lidt [eax]

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret