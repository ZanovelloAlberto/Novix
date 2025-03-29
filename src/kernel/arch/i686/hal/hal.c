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
#include <hal/physMemory_manager.h>
#include <hal/virtMemory_manager.h>

void HAL_initialize(Boot_info* info)
{
    i686_GDT_initilize();
    i686_IDT_initilize();
    i686_ISR_initialze();
    i686_IRQ_initialize();
    i686_physMmnger_initialize(info);
    i686_virtMmnger_initialize();
}