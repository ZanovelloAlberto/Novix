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
#include <stdio.h>
#include <arch/i686/physMemory_manager.h>
#include <memory.h>
#include <utility.h>

#define MAX_MEMORY_ENTRY 256
#define BLOCK_SIZEKB 4
#define BLOCK_PER_BYTE 8

typedef enum{
	FREE_BLOCK,
    USED_BLOCK,
}BITMAP_VALUE;

Memory_mapEntry g_memory4KbEntries[MAX_MEMORY_ENTRY];

uint8_t* bitmap;
uint32_t totalBlockNumber;
uint32_t bitmapSize;

int mmnger_initData(Boot_info* info)
{
    uint32_t base;

    totalBlockNumber = roundUp_div(info->memorySize, BLOCK_SIZEKB);
    bitmapSize = roundUp_div(totalBlockNumber, BLOCK_PER_BYTE);

    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(info->memoryBlockEntries[i].type == AVAILABLE)
        {
            base = info->memoryBlockEntries[i].base;
            goto Founded;
        }
    }

    return 0;

Founded:
    bitmap = (uint8_t*)base;
    info->memoryBlockEntries[info->memoryBlockCount].base = base;
    info->memoryBlockEntries[info->memoryBlockCount].length = bitmapSize;
    info->memoryBlockEntries[info->memoryBlockCount].type = RESERVED;

    info->memoryBlockCount++;

    memset(bitmap, USED_BLOCK, bitmapSize);

    return 1;
}

void memoryMap_toBlock(uint32_t memoryBlockCount)
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

void bitmap_SetBlockToFree(int block)
{
    bitmap[block / 8] &= ~(1 << block % 8);
}

void bitmap_SetBlockToUsed(int block)
{
    bitmap[block / 8] |= (1 << block % 8);
}

uint8_t test_ifUsedBlock(int block)
{
    return bitmap[block / 8] & (1 << block % 8);
}

void mmnger_initialize(Boot_info* info)
{
    uint32_t block;

    printf("initializing physical memory manager...\n\r");
    if(mmnger_initData(info) == 0)
    {
        printf("Physical Memory manager initialize failed !\n\r");
        return;
    }

    memcpy(&g_memory4KbEntries, info->memoryBlockEntries, MAX_MEMORY_ENTRY);

    memoryMap_toBlock(info->memoryBlockCount);

    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(g_memory4KbEntries[i].type == AVAILABLE)
        {
            for(int j = 0; j <= g_memory4KbEntries[i].length; j++)
            {
                block = g_memory4KbEntries[i].base + j;
                bitmap_SetBlockToFree(block);
            }
        }
    }

    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        if(g_memory4KbEntries[i].type != AVAILABLE)
        {
            for(int j = 0; j <= g_memory4KbEntries[i].length; j++)
            {
                block = g_memory4KbEntries[i].base + j;
                bitmap_SetBlockToUsed(block);
            }
        }
    }

    printf("Done !\n\r");
}

uint32_t bitmap_FirstFreeBlock()
{
    uint8_t byte;

    for(int i = 0; i < totalBlockNumber; i++)
        if(!test_ifUsedBlock(i))
            return i;

    return -1;
}