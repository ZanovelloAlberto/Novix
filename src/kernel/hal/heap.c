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
#include <hal/virtmem_manager.h>
#include <hal/heap.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define HEAP_START_ADDR 0xD0000000
#define HEAP_END_ADDR   0xDFFFFFFF

#define PAGE_SIZE 0x1000

typedef struct header_t header_t;
struct header_t{
    size_t size;
    bool isFree;
    header_t *next;
};

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

void* brk = NULL;
uint32_t lastHeapAllocatedPage;

header_t *head = NULL, *tail = NULL;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

header_t* search_freeBlock(size_t size)
{
    header_t *header = head;

    while(header != NULL)
    {
        if(header->size >= size && header->isFree)
            return header;

        header = header->next;
    }

    return NULL;
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void HEAP_initialize()
{
    colored_puts("[HAL]", VGA_COLOR_LIGHT_CYAN);
    puts("\t\tInitializing Heap manager...");

    brk = (void*)HEAP_START_ADDR;
    lastHeapAllocatedPage = HEAP_START_ADDR;

    if(!VIRTMEM_mapPage((void*)lastHeapAllocatedPage))
    {
        moveCursorTo(getCurrentLine(), 60);
        colored_puts("[Failed]\n\r", VGA_COLOR_LIGHT_RED);
        return;
    }

    moveCursorTo(getCurrentLine(), 60);
    colored_puts("[Success]\n\r", VGA_COLOR_LIGHT_GREEN);
}

void* sbrk(size_t size)
{
    void* ptr = brk;

    if(size == 0)
    {
        return brk;
    }
    else if(size > 0)
    {
        if((uint32_t)(brk+size) > HEAP_END_ADDR)
        {
            return (void*)-1;   // not enough available memory, heap is full !
        }

        if((uint32_t)(brk+size) > (lastHeapAllocatedPage + PAGE_SIZE)) // if so we will need to increase the heap size
        {
            if(!VIRTMEM_mapPage((void*)(lastHeapAllocatedPage + PAGE_SIZE)))
                return (void*)-1;   // not enough available memory, RAM is full or other error !

            lastHeapAllocatedPage += PAGE_SIZE;
        }
    }
    else
    {
        if((uint32_t)(brk+size) < HEAP_START_ADDR)
            return (void*)-1;
    }

    brk += size;
    return ptr;
}

void* kmalloc(size_t size)
{
    void *block;
    header_t *header = NULL;
    size_t totalSize = sizeof(header_t) + size;

    if(!size)
        return NULL;

    header = search_freeBlock(size);
    if(header)
    {
        header->isFree = 0;
        return ((void*)header + sizeof(header_t));

        // TODO: implement a splitting block mechanism
    }

    block = sbrk(totalSize);    // requesting memory from the heap
    if(block == (void*) -1)
        return NULL;

    //filling header information
    header = (header_t*)block;
    header->size = size;
    header->isFree = false;
    header->next = NULL;

    // now our linked list of allocated block
    if(!head)
        head = header;
    
    if(tail)
        tail->next = header;

    tail = header;

    block += sizeof(header_t);

    return block;
}