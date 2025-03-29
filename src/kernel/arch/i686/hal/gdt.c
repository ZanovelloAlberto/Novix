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

#include <hal/gdt.h>
#include <stdio.h>

Gdt_entry g_GDT[] = {
    // Null descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel code segment
    GDT_ENTRY(0, 
              0xfffff,
              GDT_ACCESS_RW_BIT_ALLOW | GDT_ACCESS_UP_DIRECTION_BIT | GDT_ACCESS_EXECUTABLE_BIT_CODE | GDT_ACCESS_DESCRIPTOR_BIT_CODEDATA | GDT_ACCESS_DPL_RING0 | GDT_ACCESS_PRESENT_BIT,
              GDT_FLAG_LONG_MODE_CLEAR | GDT_FLAG_DB_32_BIT | GDT_FLAG_GRANULARITY_PAGE_BLOCK),
    
    // Kernel data segment
    GDT_ENTRY(0, 
              0xfffff,
              GDT_ACCESS_RW_BIT_ALLOW | GDT_ACCESS_UP_DIRECTION_BIT | GDT_ACCESS_EXECUTABLE_BIT_DATA | GDT_ACCESS_DESCRIPTOR_BIT_CODEDATA | GDT_ACCESS_DPL_RING0 | GDT_ACCESS_PRESENT_BIT,
              GDT_FLAG_LONG_MODE_CLEAR | GDT_FLAG_DB_32_BIT | GDT_FLAG_GRANULARITY_PAGE_BLOCK),
};

Gdt_descriptor g_GDTdescriptor = {sizeof(g_GDT) - 1, g_GDT};

void __attribute__((cdecl)) i686_GDT_flush(Gdt_descriptor* descriptor);

void i686_GDT_initilize()
{
    printf("Initializing the GDT...\n\r");
    i686_GDT_flush(&g_GDTdescriptor);
    printf("Done !\n\r");
}