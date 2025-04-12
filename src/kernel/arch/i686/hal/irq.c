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
#include <stddef.h>
#include <hal/irq.h>
#include <hal/io.h>
#include <hal/pic.h>
#include <hal/pit.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQ_handlers[16];


void i686_IRQ_handler(Registers* regs)
{
    int irq = regs->interrupt - PIC_REMAP_OFFSET;
    
    uint8_t pic_isr = i686_PIC_readInServiceRegister();
    uint8_t pic_irr = i686_PIC_readIrqRequestRegister();

    if (g_IRQ_handlers[irq] != NULL)
    {
        // handle IRQ
        g_IRQ_handlers[irq](regs);
    }
    else
    {
        printf("Unhandled IRQ %d  ISR=%x  IRR=%x...\n", irq, pic_isr, pic_irr);
    }

    // send EOI
    i686_PIC_sendEndOfInterrupt(irq);
}

void i686_IRQ_initialize()
{
    i686_PIC_configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);
    i686_PIT_initialize();

    printf("initilazing IRQ ...\n\r");

    for(int i = 0; i < 16; i++)
        i686_ISR_registerNewHandler(PIC_REMAP_OFFSET + i, i686_IRQ_handler);

    // pit
    i686_IRQ_registerNewHandler(0, timer);

    // enable interrupts
    i686_enableInterrupts();
    printf("Done !\n\r");
}

void i686_IRQ_registerNewHandler(int irq, IRQHandler handler)
{
    g_IRQ_handlers[irq] = handler;
}