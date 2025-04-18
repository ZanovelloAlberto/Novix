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
#include <memory.h>
#include <shell.h>
#include <hal/hal.h>
#include <drivers/fdc.h>
#include <drivers/keyboard.h>
#include <boot_info.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

const char logo[] = 
"\
                 _   _            _      \n\
                | \\ | | _____   _(_)_  __\n\
                |  \\| |/ _ \\ \\ / / \\ \\/ /\n\
                | |\\  | (_) \\ V /| |>  < \n\
                |_| \\_|\\___/ \\_/ |_/_/\\_\\ \n\n\
";

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

void __attribute__((cdecl)) start(Boot_info* info)
{
    clr();

    printf("%s", logo);

    HAL_initialize(info);
    FDC_initialize();
    KEYBOARD_initialize();

    while(1)
    {
        printf("root@host> ");

        //reading
        shellRead();

        //parsing
        shellParse();
        
        //execute
        shellExecute();
    }

end:
    for (;;);
}