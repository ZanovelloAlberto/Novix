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
#include <hal/io.h>
#include <hal/pit.h>
#include <debug.h>
#include <drivers/vga_text.h>
#include <stddef.h>
#include <memory.h>
#include <shell.h>
#include <hal/hal.h>
#include <drivers/fdc.h>
#include <drivers/keyboard.h>
#include <vfs/vfs.h>
#include <boot_info.h>
#include <scheduler/multitask.h>
#include <memmgr/physmem_manager.h>
#include <memmgr/memory_manager.h>
#include <memmgr/virtmem_manager.h>
#include <memmgr/heap.h>
#include <memmgr/vmalloc.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

/*volatile const char logo[] = 
"\
                         _   _            _      \n\
                        | \\ | | _____   _(_)_  __\n\
                        |  \\| |/ _ \\ \\ / / \\ \\/ /\n\
                        | |\\  | (_) \\ V /| |>  < \n\
                        |_| \\_|\\___/ \\_/ |_/_/\\_\\ \n\n\
";*/

const char logo[] = 
"\
            __    __   ______   __     __  ______  __    __ \n\
            |  \\  |  \\ /      \\ |  \\   |  \\|      \\|  \\  |  \\\n\
            | $$\\ | $$|  $$$$$$\\| $$   | $$ \\$$$$$$| $$  | $$\n\
            | $$$\\| $$| $$  | $$| $$   | $$  | $$   \\$$\\/  $$\n\
            | $$$$\\ $$| $$  | $$ \\$$\\ /  $$  | $$    >$$  $$ \n\
            | $$\\$$ $$| $$  | $$  \\$$\\  $$   | $$   /  $$$$\\ \n\
            | $$ \\$$$$| $$__/ $$   \\$$ $$   _| $$_ |  $$ \\$$\\\n\
            | $$  \\$$$ \\$$    $$    \\$$$   |   $$ \\| $$  | $$\n\
             \\$$   \\$$  \\$$$$$$      \\$     \\$$$$$$ \\$$   \\$$\n\n\
";

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

mutex_t log;

void taskC();
void taskA()
{
    //unlock_sheduler();
    for(int i = 0; i < 5; i++)
    {
        sleep(600);
        log_info("taskA", "A is running !");
        //yield();
    }

    create_process(taskC, false);
    terminate_task();
}

void taskD();
void taskB()
{
    //unlock_sheduler();

    for(int i = 0; i < 10; i++)
    {
        sleep(400);
        log_info("taskB", "B is running !");
        //yield();
    }

    create_process(taskD, false);
    terminate_task();
}

void taskC()
{
    //unlock_sheduler();

    for(int i = 0; i < 8; i++)
    {
        sleep(400);
        log_info("taskC", "C is running !");

        //yield();
    }
    create_process(taskA, false);
    terminate_task();
}

void taskD()
{
    //unlock_sheduler();

    for(int i = 0; i < 13; i++)
    {
        sleep(200);
        log_info("taskD", "D is running !");

        //yield();
    }
    create_process(taskB, false);
    terminate_task();
}

void init_process()
{
    VGA_clr();
    VGA_puts(logo);
    
    FDC_initialize();
    KEYBOARD_initialize();
    VFS_init();

    if(VFS_mount("fat12", "/") != VFS_OK)
        printf("error while mounting at /!\n");

    create_process(taskA, false);
    create_process(taskB, false);
    create_process("/userprog.bin", true);

    terminate_task();
}

void __attribute__((cdecl)) start(Boot_info* info)
{
    HAL_initialize(info);
    PHYSMEM_initialize(info);
    VIRTMEM_initialize();
    HEAP_initialize();
    VMALLOC_initialize();
    initialize_multitasking();
    create_process(init_process, false);
    enable_multitasking();    // preemptive multitasking
    //yield();

    for(;;)
    {
        HLT();
        //yield();
    }
        

    while(1)
    {
        VGA_coloredPuts("root@host> ", VGA_COLOR_WHITE);
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
