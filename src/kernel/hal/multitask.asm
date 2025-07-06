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

[bits 32]

global context_switch
context_switch:
    TASK_ESP_OFFSET equ 4
    TASK_CR3_OFFSET equ 0

    mov eax, [esp+4]    ; current process
    mov edx, [esp+8]    ; next process

    ; Save the context of the current task
    ; Only save the registers that the called function
    ; must preserve (callee-saved registers) and flags
    push ebp
    push ebx
    push esi
    push edi
    pushf
    
    ; Save ESP of the current task
    mov [eax + TASK_ESP_OFFSET], esp  ; eax = current task

    ; Load ESP of the new task
    mov esp, [edx + TASK_ESP_OFFSET] ; edx = next task

    mov esi, [eax + TASK_CR3_OFFSET]
    mov edi, [edx + TASK_CR3_OFFSET]
    cmp edi, esi
    je .sameVAS

    ; switch pdbr (Page Directory Base Register)
    mov cr3, edi

.sameVAS:
    ; Restore the context of the new task
    popf
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret  ; Return to the new task!