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
#include "stdio.h"
#include "disk.h"
#include "fat.h"
#include "x86.h"
#include "memdetect.h"

void* Kernel = (void*)0x100000;
typedef void (*KernelStart)(Boot_info* info);

Boot_info g_info;

void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    clr();
    
    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    if (!FAT_Initialize(&disk))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    FAT_LoadFile(&disk, "/kernel.bin", Kernel);

    memoryDetect(&g_info);
    g_info.bootDrive = bootDrive;

    // execute kernel
    KernelStart kernelStart = (KernelStart)Kernel;
    kernelStart(&g_info);

end:
    for (;;);
}
