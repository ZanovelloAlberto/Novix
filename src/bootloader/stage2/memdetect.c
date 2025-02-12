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

#define MAX_MEMORY_ENTRY 256
#include "x86.h"

Memory_mapEntry g_memoryBlockEntries[MAX_MEMORY_ENTRY];

void memoryDetect(Boot_info* info)
{
    Memory_mapEntry memoryBlockEntry;

    uint32_t continuation = 0;
    uint32_t memoryBlockCount = 0;
    uint16_t ret = 0;

    info->memorySize = x86_Get_MemorySize();

    do
    {
        ret = x86_Get_MemoryMapEntry(&memoryBlockEntry, &continuation);
        g_memoryBlockEntries[memoryBlockCount] = memoryBlockEntry;
        memoryBlockCount++;
    }while(ret != 0 && continuation != 0 );

    info->memoryBlockCount = memoryBlockCount;
    info->memoryBlockEntries = g_memoryBlockEntries;
}