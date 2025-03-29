/*
 * Copyright (C) 2025,  Novice
 *
 * This file is part of the Novix software.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <hal/idt.h>
#include <stdio.h>

Idt_gate g_IDT[256];
Idt_descriptor g_IDTdescriptor = {sizeof(g_IDT) - 1, g_IDT};

void __attribute__((cdecl)) i686_IDT_flush(Idt_descriptor* descriptor);

void i686_IDT_setGate(int interrupt, void* offset, uint8_t attribute)
{
    g_IDT[interrupt].offset_low     = (uint32_t)offset & 0XFFFF;
    g_IDT[interrupt].segment        = 0X08;
    g_IDT[interrupt].reserved       = 0;
    g_IDT[interrupt].attribute      = attribute;
    g_IDT[interrupt].offset_high    = ((uint32_t)offset >> 16) & 0xFFFF;
}

void i686_IDT_initilize()
{
    printf("Initializing the IDT...\n\r");
    i686_IDT_flush(&g_IDTdescriptor);
    printf("Done !\n\r");
}