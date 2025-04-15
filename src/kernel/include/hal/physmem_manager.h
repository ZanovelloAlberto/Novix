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
#include <boot_info.h>
#include <stdbool.h>

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void PHYSMEM_initialize(Boot_info* info);
void PHYSMEM_freeBlock(void* ptr);
void* PHYSMEM_AllocBlock();
void* PHYSMEM_AllocBlocks(uint8_t blocks);
void PHYSMEM_freeBlocks(void* ptr, uint8_t size);
void PHYSMEM_getMemoryInfo(uint32_t* bitmapSizeOut, uint32_t* totalBlockNumberOut, uint32_t* totalFreeBlockOut, uint32_t* totalUsedBlockOut);