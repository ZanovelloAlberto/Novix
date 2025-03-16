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

#include <stdio.h>
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/physMemory_manager.h>
#include <boot_info.h>

void timer(Registers* regs)
{
    //putc('.');
}

const char logo[] = 
"\
                 _   _            _      \n\
                | \\ | | _____   _(_)_  __\n\
                |  \\| |/ _ \\ \\ / / \\ \\/ /\n\
                | |\\  | (_) \\ V /| |>  < \n\
                |_| \\_|\\___/ \\_/ |_/_/\\_\\ \n\n\
";

void __attribute__((cdecl)) start(Boot_info* info)
{
    i686_IRQ_registerNewHandler(0, timer);
    clr();

    printf("%s", logo);

    HAL_initialize(info);

    printf("bootDrive: %d\n", info->bootDrive);
    printf("memory size: 0x%lxKb\n", info->memorySize);
    printf("count: %d\n\r", info->memoryBlockCount);
    
    for(int i = 0; i < info->memoryBlockCount; i++)
    {
        printf("base: 0x%llx, length: 0x%llx, type: 0x%x\n", info->memoryBlockEntries[i].base, info->memoryBlockEntries[i].length, info->memoryBlockEntries[i].type);
    }

    uint32_t totalBlockNumber;
    uint32_t totalFreeBlock;
    uint32_t totalUsedBlock;
    uint32_t bitmapSize;

    i686_mmnger_getMemoryInfo(&bitmapSize, &totalBlockNumber, &totalFreeBlock, &totalUsedBlock);

    printf("bitmap Size: %d, total Block Number: %d, total Free Block: %d, total Used Block: %d\n\r", bitmapSize, totalBlockNumber, totalFreeBlock, totalUsedBlock);

    void* ptr = i686_physMemoryAllocBlock();
    printf("ptr allocated at: 0x%x\n\r", ptr);

    i686_mmnger_getMemoryInfo(&bitmapSize, &totalBlockNumber, &totalFreeBlock, &totalUsedBlock);

    printf("bitmap Size: %d, total Block Number: %d, total Free Block: %d, total Used Block: %d\n\r", bitmapSize, totalBlockNumber, totalFreeBlock, totalUsedBlock);

    printf("freeing ptr\n\r");
    i686_physMemoryfreeBlock(ptr);

    i686_mmnger_getMemoryInfo(&bitmapSize, &totalBlockNumber, &totalFreeBlock, &totalUsedBlock);

    printf("bitmap Size: %d, total Block Number: %d, total Free Block: %d, total Used Block: %d\n\r", bitmapSize, totalBlockNumber, totalFreeBlock, totalUsedBlock);

end:
    for (;;);
}