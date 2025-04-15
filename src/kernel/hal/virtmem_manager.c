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
#include <stddef.h>
#include <stdio.h>
#include <hal/memory_manager.h>
#include <hal/physmem_manager.h>
#include <hal/virtmem_manager.h>
#include <memory.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define PAGE_ADD_ATTRIBUTE(page_entry, flags)     page_entry | flags
#define PAGE_SET_FRAME(page_entry, frame)         page_entry | frame

#define PTE_INDEX(virt_addr) (virt_addr >> 12) & 0x3ff
#define PDE_INDEX(virt_addr) (virt_addr >> 22) & 0x3ff

typedef enum {
    PTE_PAGE_PRESENT        = 0X1,
    PTE_PAGE_WRITE          = 0X2,
    PTE_PAGE_KERNEL_MODE    = 0X0,
    PTE_PAGE_USER_MODE      = 0X4,
}PTE_FLAGS;

typedef enum {
    PDE_PRESENT             = 0X01,
    PDE_WRITE               = 0X02,
    PDE_KERNEL_MODE         = 0X00,
    PDE_USER_MODE           = 0X04,
    PDE_PWT                 = 0X08,
    PDE_PCD                 = 0X10,
    PDE_4KBPAGE             = 0X00,
    PDE_4MBPAGE             = 0X80,
}PDE_FLAGS;

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

int VIRTMEM_allocPage(PTE* entry, uint32_t flags)
{
    PTE page = (PTE)PHYSMEM_AllocBlock(); // allocate a free physical frame
    if(!page)
        return 0;

    *entry = PAGE_ADD_ATTRIBUTE(page, flags);

    return 1;
}

void VIRTMEM_freePage(PTE* entry)
{
    void* ptr = (void*)(*entry & 0xFFF00000);
    PHYSMEM_freeBlock(ptr);

    *entry = 0x0; // page not present
}

void VIRTMEM_mapPage (void* virt)
{
    if((uint32_t)virt >= 0xFFC00000) // arealdy used by recursive mapping
        return;

    PDE* page_directory = (PDE*)0xFFFFF000; // virtuall addresse of the page directory
    
    uint32_t pageTableIndex = PDE_INDEX((uint32_t)virt);
    PTE* page_table = (PTE*)(0xFFC00000 + (pageTableIndex << 12));   // virtuall addresse of the page table

    if((page_directory[pageTableIndex] & PDE_PRESENT) != PDE_PRESENT)
    {
        void* frame = PHYSMEM_AllocBlock(); // physical address of the page table
        if(!frame)
            return;

        page_directory[pageTableIndex] = PAGE_ADD_ATTRIBUTE((uint32_t)frame, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);
        memset(page_table, 0, 0x1000);
    }

    uint32_t pageEntryIndex = PTE_INDEX((uint32_t)virt);
    if((page_table[pageEntryIndex] & PTE_PAGE_PRESENT) == PTE_PAGE_PRESENT)
        return; // page already mapped nothing to do

    VIRTMEM_allocPage(&page_table[pageEntryIndex], PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE);
}

void VIRTMEM_initialize()
{
    printf("initializing virtual memory manager...\n\r");

    // allocate default page directory table
    PDE* page_directory = PHYSMEM_AllocBlock();

    // allocate first page table
    PTE* table_0 = PHYSMEM_AllocBlock();

    // allocates 3gb page table
    PTE* table_768 = PHYSMEM_AllocBlock();

    if(page_directory == NULL || table_0 == NULL || table_768 == NULL)
    {
        printf("Virtual Memory manager initialize failed !\n\r");
        return;
    }

    // 1st 4mb are idenitity mapped
   for (int i=0, frame=0x0, virt=0x00000000; i<1024; i++, frame+=4096, virt+=4096)
   {

      // create a new page
      PTE page = 0;
      page = PAGE_ADD_ATTRIBUTE(page, PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE);
      page = PAGE_SET_FRAME(page, frame);

      // ...and add it to the page table
      table_0[PTE_INDEX(virt)] = page;
   }

   // map 1mb to 3gb (where we are at)
   for (int i=0, frame=0x100000, virt=0xc0000000; i<1024; i++, frame+=4096, virt+=4096)
   {

      // create a new page
      PTE page = 0;
      page = PAGE_ADD_ATTRIBUTE(page, PTE_PAGE_PRESENT | PTE_PAGE_WRITE | PTE_PAGE_KERNEL_MODE);
      page = PAGE_SET_FRAME(page, frame);

      // ...and add it to the page table
      table_768[PTE_INDEX(virt)] = page;
   }

    // clear and initialize directory table
    memset(page_directory, 0, 0x1000);
    page_directory[PDE_INDEX(0x0)] = PAGE_ADD_ATTRIBUTE((uint32_t)table_0, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);
    page_directory[PDE_INDEX(0xc0000000)] = PAGE_ADD_ATTRIBUTE((uint32_t)table_768, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);

    // recursive mapping here !
    page_directory[1023] = PAGE_ADD_ATTRIBUTE((uint32_t)page_directory, PDE_PRESENT | PDE_WRITE | PDE_KERNEL_MODE);

    switchPDBR(page_directory);
    enablePaging();    // just in case ...
    printf("Done !\n\r");
}