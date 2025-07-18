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

global switch_to_usermode
switch_to_usermode:
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; set up the stack frame iret expects
    push (4 * 8) | 3    ; user SS
    mov eax, [ebp+8]
    push eax            ; ESP
    pushf               ; EFLAGS
    push (3 * 8) | 3    ; user CS
    mov eax, [ebp+12]
    push eax            ; EIP

    iret

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret