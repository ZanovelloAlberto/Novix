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

#pragma once
#include <stdint.h>

typedef struct{
    uint64_t base;
    uint64_t length;
    uint16_t type;
    uint16_t acpi;
}Memory_mapEntry;

typedef enum{
	AVAILABLE 	= 1,
	RESERVED 	= 2,
	ACPI 		= 3,
	ACPI_NVS 	= 4,
}MEMORY_TYPE;

typedef struct{
    uint16_t bootDrive;
    uint32_t memorySize;
    uint32_t memoryBlockCount;
    Memory_mapEntry* memoryBlockEntries;
}Boot_info;