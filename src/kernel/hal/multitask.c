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
#include <debug.h>
#include <hal/heap.h>
#include <hal/vmalloc.h>
#include <hal/memory_manager.h>
#include <memory.h>
#include <hal/gdt.h>
#include <hal/multitask.h>

uint64_t pids;
process_t* idle;
process_t* process_list;
process_t* tail_list;
process_t* current_process;

void add_process(process_t* proc)
{
    if(!process_list)
        process_list = proc;
    
    if(tail_list)
        tail_list->next = proc;

    tail_list = proc;
}

void delete_process(process_t* proc)
{
    if(proc->stack)
        vfree(proc->stack);

    if(proc->stack0)
        vfree(proc->stack0);

    // free address space

    kfree(proc);
}

process_t* PROCESS_getCurrent()
{
    return current_process;
}

process_t* PROCESS_scheduleNext()
{
    if(current_process == idle && process_list != NULL)
        current_process = process_list;

    while(process_list != NULL)
    {
        process_t* prev_process = current_process;

        if(current_process->next != NULL)
            current_process = current_process->next;
        else
            current_process = process_list;

        if(current_process->status == DEAD)
        {
            if(current_process == process_list)
                process_list = current_process->next;

            process_t* dead_process = current_process;

            if(prev_process->next != NULL)
                prev_process->next = current_process->next;

            current_process = current_process->next;

            delete_process(dead_process);

            continue;
        }

        break;
    }

    return current_process != NULL ? current_process : idle;
}

void yield()
{
    process_t* current = PROCESS_getCurrent();
    process_t* next = PROCESS_scheduleNext();

    if(current != next)
    {
        if(next->stack0 != NULL)    // if it's a usermode process
            TSS_setKernelStack((uint32_t)next->stack0 + 0x1000);
        
        current->status = READY;
        next->status = RUNNING;

        context_switch(current, next);
    }
}

void initialize_multitasking()
{
    process_list = NULL;
    tail_list = NULL;
    current_process = NULL;

    // create the firs process which is the idle process
    idle = kmalloc(sizeof(process_t));

    idle->stack = NULL;     // no need to create a new stack
    idle->stack0 = NULL;    // because it's a kernel process

    idle->esp = NULL;       // this will be filled automatically when a context switch occurs

    idle->page_directory = getPDBR();
    idle->id = pids++;
    idle->status = READY;
    idle->next = NULL;

    current_process = idle;
}

void create_kernel_process(void (*task)(void))
{
    process_t* proc = kmalloc(sizeof(process_t));

    proc->page_directory = getPDBR();

    proc->stack = vmalloc(1);
    proc->stack0 = NULL;    // it's a kernel task already

    proc->esp = proc->stack + 0x1000 - 4;
    *(uint32_t*)proc->esp = (uint32_t)task; // return address after context switch
    proc->esp -= (4 * 4);   // pushed register

    proc->id = pids++;
    proc->status = READY;
    proc->next = NULL;

    add_process(proc);
}