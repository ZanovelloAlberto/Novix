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

#include <hal/hal.h>
#include <hal/gdt.h>
#include <hal/idt.h>
#include <hal/isr.h>
#include <hal/irq.h>
#include <hal/physmem_manager.h>
#include <hal/virtmem_manager.h>
#include <hal/heap.h>
#include <hal/vmalloc.h>
#include <hal/dma.h>
#include <hal/syscall.h>

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void HAL_initialize(Boot_info* info)
{
    GDT_initilize();
    IDT_initilize();
    ISR_initialze();
    IRQ_initialize();
    PHYSMEM_initialize(info);
    VIRTMEM_initialize();
    HEAP_initialize();
    VMALLOC_initialize();
    DMA_enable();
    SYSCALL_initialize();
}