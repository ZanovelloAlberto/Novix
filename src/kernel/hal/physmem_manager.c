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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <hal/physmem_manager.h>
#include <memory.h>
#include <utility.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define MAX_MEMORY_ENTRY 256
#define BLOCK_SIZEKB 4
#define BLOCK_PER_BYTE 8

typedef enum{
	FREE_BLOCK, // 0
    USED_BLOCK, // 1
}BITMAP_VALUE;

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

// memory map but in 4kb size
Memory_mapEntry g_memory4KbEntries[MAX_MEMORY_ENTRY];

// useful data for our memory manager
uint8_t* bitmap;
uint32_t totalBlockNumber   = 0;
uint32_t totalFreeBlock     = 0;
uint32_t totalUsedBlock     = 0;
uint32_t bitmapSize;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

uint8_t PHYSMEM_checkIfBlockUsed(int block);
int PHYSMEM_initData(Boot_info* info);
void PHYSMEM_memoryMapToBlock(uint32_t memoryBlockCount);
void PHYSMEM_setBlockToFree(uint32_t block);
void PHYSMEM_setBlockToUsed(int block);
uint8_t PHYSMEM_checkIfBlockUsed(int block);
uint32_t PHYSMEM_firstFreeBlock();
uint32_t PHYSMEM_firstFreeBlockFrom(uint32_t position);

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

// this function initialize some of the useful data of the memory manager
int PHYSMEM_initData(Boot_info* info)
{
    uint32_t base;

    totalBlockNumber = roundUp_div(info->memorySize, BLOCK_SIZEKB);
    bitmapSize = roundUp_div(totalBlockNumber, BLOCK_PER_BYTE);

    // here we are trying to find a free block of memory for the bitmap
    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(info->memoryBlockEntries[i].type == AVAILABLE && info->memoryBlockEntries[i].length >= bitmapSize)
        {
            base = info->memoryBlockEntries[i].base;
            goto Found;
        }
    }

    return 0;

Found:
    bitmap = (uint8_t*)base;

    // we need to add a new reserved region to our memory map 
    info->memoryBlockEntries[info->memoryBlockCount].base = base;
    info->memoryBlockEntries[info->memoryBlockCount].length = bitmapSize;
    info->memoryBlockEntries[info->memoryBlockCount].type = RESERVED;

    info->memoryBlockCount++;

    // the memory map to the 4kb size array
    memcpy(&g_memory4KbEntries, info->memoryBlockEntries, MAX_MEMORY_ENTRY);

    // initialy we mark the whole memory as used
    memset(bitmap, 0b11111111, bitmapSize);

    return 1;
}

// this convert the memory map entry to 4kb size
void PHYSMEM_memoryMapToBlock(uint32_t memoryBlockCount)
{
	for(int i = 0; i < memoryBlockCount; i++)
	{
		if(g_memory4KbEntries[i].type == AVAILABLE)
		{
			g_memory4KbEntries[i].base = roundUp_div(g_memory4KbEntries[i].base, BLOCK_SIZEKB * 0x400);
			g_memory4KbEntries[i].length = g_memory4KbEntries[i].length / (BLOCK_SIZEKB * 0x400);
		}else{
			g_memory4KbEntries[i].base = g_memory4KbEntries[i].base / (BLOCK_SIZEKB * 0x400);
			g_memory4KbEntries[i].length = roundUp_div(g_memory4KbEntries[i].length, BLOCK_SIZEKB * 0x400);
		}
	}
}

void PHYSMEM_setBlockToFree(uint32_t block)
{
    bitmap[block / 8] ^= (1 << block % 8);
}

void PHYSMEM_setBlockToUsed(int block)
{
    bitmap[block / 8] |= (1 << block % 8);
}

uint8_t PHYSMEM_checkIfBlockUsed(int block)
{
    return bitmap[block / 8] & (1 << block % 8);
}

uint32_t PHYSMEM_firstFreeBlock()
{
    for(int i = 0; i < totalBlockNumber; i++)
        if(PHYSMEM_checkIfBlockUsed(i) == 0)
            return i;

    return -1;
}

uint32_t PHYSMEM_firstFreeBlockFrom(uint32_t position)
{
    for(int i = position; i < totalBlockNumber; i++)
        if(PHYSMEM_checkIfBlockUsed(i) == 0)
            return i;

    return -1;
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void PHYSMEM_getMemoryInfo(uint32_t* bitmapSizeOut, uint32_t* totalBlockNumberOut, uint32_t* totalFreeBlockOut, uint32_t* totalUsedBlockOut)
{
    *bitmapSizeOut = bitmapSize;
    *totalBlockNumberOut = totalBlockNumber;
    *totalUsedBlockOut = totalUsedBlock;
    *totalFreeBlockOut = totalFreeBlock;
}

void PHYSMEM_initialize(Boot_info* info)
{
    uint32_t block;

    puts("initializing physical memory manager...\n\r");
    if(PHYSMEM_initData(info) == 0)
    {
        puts("Physical Memory manager initialize failed !\n\r");
        return;
    }

    PHYSMEM_memoryMapToBlock(info->memoryBlockCount);

    /*
    * for the sake of ...
    * we need to first mark the availabe memory then the reserved ones
    * this prevent things like overlaping memory block
    */

    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(g_memory4KbEntries[i].type == AVAILABLE)
        {
            for(int j = 0; j < g_memory4KbEntries[i].length; j++)
            {
                block = g_memory4KbEntries[i].base + j;
                PHYSMEM_setBlockToFree(block);
            }
        }
    }

    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(g_memory4KbEntries[i].type != AVAILABLE)
        {
            for(int j = 0; j < g_memory4KbEntries[i].length; j++)
            {
                block = g_memory4KbEntries[i].base + j;
                PHYSMEM_setBlockToUsed(block);
            }
        }
    }

    for(int i = 0; i < totalBlockNumber; i++)
    {
        if(PHYSMEM_checkIfBlockUsed(i) == 0)
            totalFreeBlock++;
        else
            totalUsedBlock++;
    }

    puts("Done !\n\r");
}

void* PHYSMEM_AllocBlock()
{
    uint32_t block = PHYSMEM_firstFreeBlock();

    if(block == -1)
        return NULL;
    PHYSMEM_setBlockToUsed(block);
    totalUsedBlock++;
    totalFreeBlock--;

    return (void*)(block * BLOCK_SIZEKB * 0x400);
}

void* PHYSMEM_AllocBlocks(uint8_t block_size)
{
    uint32_t index;
    uint32_t block_addr;
    uint8_t count;

    uint16_t debug = 0;

    if(block_size > totalFreeBlock)
        return NULL;
    
    index = PHYSMEM_firstFreeBlock();
    block_addr = index;
    count = 1; // we already have one block

    while (index < totalBlockNumber)
    {
        if(count >= block_size)
        {
            for(int i = 0; i < count; i++)
            {
                PHYSMEM_setBlockToUsed(block_addr + i);
                totalUsedBlock++;
                totalFreeBlock--;
            }
            
            return (void*)(block_addr * BLOCK_SIZEKB * 0x400);
        }

        index++;
        if(PHYSMEM_checkIfBlockUsed(index))
        {
            count = 1;
            index = PHYSMEM_firstFreeBlockFrom(index);
            block_addr = index;
        }

        count++;
    }
    
    return NULL;
}

void PHYSMEM_freeBlock(void* ptr)
{
    if(!ptr)
        return;

    uint32_t block = (uint32_t)ptr / (BLOCK_SIZEKB * 0x400);

    PHYSMEM_setBlockToFree(block);
    totalUsedBlock--;
    totalFreeBlock++;
}

void PHYSMEM_freeBlocks(void* ptr, uint8_t size)
{
    if(!ptr)
        return;

    uint32_t block = (uint32_t)ptr / (BLOCK_SIZEKB * 0x400);

    for(int i = 0; i < size; i++)
    {
        PHYSMEM_setBlockToFree(block + i);
        totalUsedBlock--;
        totalFreeBlock++;
    }
}