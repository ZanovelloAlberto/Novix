; Copyright (C) 2025,  Novice
;
; This file is part of the Novix software.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.



; out:
; ax - state (0 - disabled, 1 - enabled)
get_a20_state:
    pushf
    push si
    push di
    push ds
    push es

    mov ax, 0x0000 ; 0x0000:0x0500(0x00000500) -> ds:si
    mov ds, ax
    mov si, 0x0500

    not ax ; 0xffff:0x0510(0x00100500) -> es:di
    mov es, ax
    mov di, 0x0510

    mov al, [ds:si] ; save old values
    mov byte [.BufferBelowMB], al

    mov al, [es:di]
    mov byte [.BufferOverMB], al

    mov ah, 1
    mov byte [ds:si], 0
    mov byte [es:di], 1
    mov al, [ds:si]
    cmp al, [es:di] ; check byte at address 0x0500 != byte at address 0x100500

    jne .exit
    dec ah
.exit:
    mov al, [.BufferBelowMB] ; restore old values
    mov [ds:si], al
    mov al, [.BufferOverMB]
    mov [es:di], al

    shr ax, 8 ; move result from ah to al register and clear ah
    
    pop es
    pop ds
    pop di
    pop si
    popf
    ret
.BufferBelowMB: db 0
.BufferOverMB: db 0


A20WaitInput:
    ; wait until status bit 2 (input buffer) is 0
    ; by reading from command port, we read status byte
    in al, KbdControllerCommandPort
    test al, 2
    jnz A20WaitInput
    ret

A20WaitOutput:
    ; wait until status bit 1 (output buffer) is 1 so it can be read
    in al, KbdControllerCommandPort
    test al, 1
    jz A20WaitOutput
    ret

EnableA20:

    call get_a20_state
    or ax, ax
    jnz .enabled

    ; disable keyboard
    call A20WaitInput
    mov al, KbdControllerDisableKeyboard
    out KbdControllerCommandPort, al

    ; read control output port
    call A20WaitInput
    mov al, KbdControllerReadCtrlOutputPort
    out KbdControllerCommandPort, al

    call A20WaitOutput
    in al, KbdControllerDataPort
    push eax

    ; write control output port
    call A20WaitInput
    mov al, KbdControllerWriteCtrlOutputPort
    out KbdControllerCommandPort, al
    
    call A20WaitInput
    pop eax
    or al, 2                                    ; bit 2 = A20 bit
    out KbdControllerDataPort, al

    ; enable keyboard
    call A20WaitInput
    mov al, KbdControllerEnableKeyboard
    out KbdControllerCommandPort, al

    call A20WaitInput

.enabled:
    ret

KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1
