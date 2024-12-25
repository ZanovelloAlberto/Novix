#include <stdint.h>
#include "stdio.h"
#include "disk.h"
#include "fat.h"

void* Kernel = (void*)0x100000;

typedef void (*KernelStart)();

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

    // execute kernel
    KernelStart kernelStart = (KernelStart)Kernel;
    kernelStart();

end:
    for (;;);
}
