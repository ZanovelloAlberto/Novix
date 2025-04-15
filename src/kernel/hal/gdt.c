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

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

typedef struct{
    uint16_t limit_low;                 // limit (bits 0-15)
    uint16_t base_low;                  // base (bits 0-15)
    uint8_t base_middle;                // base (bits 16-23)
    uint8_t access_byte;                // access
    uint8_t highLimit_flags;            // limit (bits 16-19) | flags
    uint8_t base_high;                  // base (bits 24-31)
} __attribute__((packed)) Gdt_entry;

typedef struct{
    uint16_t size;
    Gdt_entry* offset;
}__attribute__((packed)) Gdt_descriptor;

/*
* These enumerations are here to avoid magic numbers in the code as much as possible!
*/

typedef enum{
    //ACCESS BIT will be set by the cpu

    GDT_ACCESS_RW_BIT_ALLOW             = 0X02,
    GDT_ACCESS_RW_BIT_NOTALLOW          = 0X00,

    GDT_ACCESS_UP_DIRECTION_BIT         = 0X00,
    GDT_ACCESS_DOWN_DIRECTION_BIT       = 0X04,

    GDT_ACCESS_EXECUTABLE_BIT_CODE      = 0X08,
    GDT_ACCESS_EXECUTABLE_BIT_DATA      = 0X00,

    GDT_ACCESS_DESCRIPTOR_BIT_SYSTEM    = 0X00,
    GDT_ACCESS_DESCRIPTOR_BIT_CODEDATA  = 0X10,

    GDT_ACCESS_DPL_RING0                = 0X00,
    GDT_ACCESS_DPL_RING1                = 0X20,
    GDT_ACCESS_DPL_RING2                = 0X40,
    GDT_ACCESS_DPL_RING3                = 0X60,

    GDT_ACCESS_PRESENT_BIT              = 0x80,
}GDT_ACCESS_BYTE;

typedef enum{
    //reseved

    GDT_FLAG_LONG_MODE_SET              = 0X20,
    GDT_FLAG_LONG_MODE_CLEAR            = 0X00,

    GDT_FLAG_DB_16_BIT                  = 0X00,
    GDT_FLAG_DB_32_BIT                  = 0X40,

    GDT_FLAG_GRANULARITY_BYTE_BLOCK     = 0X00,
    GDT_FLAG_GRANULARITY_PAGE_BLOCK     = 0X80,
}GDT_FLAG_BYTE;

// helper macro !
#define GDT_ENTRY(base, limit, access, flags) {             \
    limit & 0xffff,                                         \
    base & 0xffff,                                          \
    ((base >> 16) & 0xff),                                  \
    access,                                                 \
    ((limit >> 16) & 0xf) | (flags & 0xf0),                 \
    (base >> 24) & 0xff                                     \
}

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

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

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

void __attribute__((cdecl)) GDT_flush(Gdt_descriptor* descriptor);

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void GDT_initilize()
{
    printf("Initializing the GDT...\n\r");
    GDT_flush(&g_GDTdescriptor);
    printf("Done !\n\r");
}