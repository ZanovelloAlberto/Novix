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

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    clr();

    printf("%s", logo);

    HAL_initialize();

    i686_IRQ_registerNewHandler(0, timer);
    printf("bootdrive: %d !\n\r", bootDrive);

end:
    for (;;);
}