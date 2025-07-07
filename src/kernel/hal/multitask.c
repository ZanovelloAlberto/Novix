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
#include <hal/pit.h>
#include <hal/io.h>
#include <memmgr/heap.h>
#include <memmgr/vmalloc.h>
#include <memmgr/memory_manager.h>
#include <memmgr/virtmem_manager.h>
#include <memory.h>
#include <hal/gdt.h>
#include <hal/multitask.h>

uint64_t pids = 0;

process_t* idle;
process_t* cleaner_process;

// ready list
process_t* first_READY_process = NULL;
process_t* last_READY_process = NULL;

process_t* current_process;

// dead list
process_t* first_DEAD_process = NULL;
process_t* last_DEAD_process = NULL;

uint32_t disable_irq_count = 0;

void lock_sheduler()
{
    disableInterrupts();
    disable_irq_count++;
}

void unlock_sheduler()
{
    if(disable_irq_count <= 0)
        return;

    disable_irq_count--;

    if(disable_irq_count == 0)
        enableInterrupts();
}

void add_READY_process(process_t* proc)
{
    lock_sheduler();

    if(first_READY_process == NULL)
        first_READY_process = proc;
    
    if(last_READY_process != NULL)
        last_READY_process->next = proc;

    last_READY_process = proc;
    last_READY_process->status = READY;
    last_READY_process->next = NULL;

    unlock_sheduler();
}

void add_DEAD_process(process_t* proc)
{
    lock_sheduler();

    if(first_DEAD_process == NULL)
        first_DEAD_process = proc;
    
    if(last_DEAD_process != NULL)
        last_DEAD_process->next = proc;

    last_DEAD_process = proc;
    last_DEAD_process->status = DEAD;
    last_DEAD_process->next = NULL;

    unlock_sheduler();
}

process_t* schedule_next_process()
{
    lock_sheduler();    // dont forget to unlock   

    if(current_process->status == RUNNING && current_process != idle)
    {
        add_READY_process(current_process); // never add the idle or blocked and dead task
    }
        

    if(first_READY_process == NULL)
    {
        current_process = idle;
        current_process->status = RUNNING;
        unlock_sheduler();
        return current_process;
    }
    
    current_process = first_READY_process;
    current_process->status = RUNNING;

    if(first_READY_process == last_READY_process)
    {
        first_READY_process = NULL;
        last_READY_process = NULL;
        unlock_sheduler();
        return current_process;
    }

    first_READY_process = first_READY_process->next;
    unlock_sheduler();
    return current_process;
    
}

void yield()
{
    lock_sheduler();

    process_t* prev = current_process;
    process_t* next = schedule_next_process();

    if(prev != next)
    {   
        if(next->stack0 != NULL)    // if it's a usermode process
            TSS_setKernelStack((uint32_t)next->stack0 + 0x1000);

        context_switch(prev, next);
    }

    unlock_sheduler();
}

void block_current_task()
{
    lock_sheduler();

    current_process->status = BLOCKED;

    unlock_sheduler();
}

void unblock_task(process_t* proc)
{
    lock_sheduler();

    // add it to the front list so it can be executed right away
    proc->next = first_READY_process;
    first_READY_process = proc;
    first_READY_process->status = READY;

    if(last_READY_process == NULL)
        last_READY_process = first_READY_process;

    unlock_sheduler();
}

void process_prelaunch()
{
    void (*task)(void) = (void*)(*(uint32_t*)(current_process->esp + (4 * 6)));

    log_warn("prelaunch", "cr3 of the current task 0x%x", current_process->page_directory);
    log_warn("prelaunch", "task 0x%x", task);
    unlock_sheduler();

    task();
}

void create_kernel_process(void (*task)(void))
{
    process_t* proc = kmalloc(sizeof(process_t));

    proc->page_directory = getPDBR();
    //proc->page_directory = VIRTMEM_createAddressSpace();

    proc->stack = vmalloc(1);
    proc->stack0 = NULL;    // it's a kernel task already

    proc->esp = proc->stack + 0x1000 - 4;
    *(uint32_t*)proc->esp = (uint32_t)task; // argument for process_prelaunch

    proc->esp -= 4;
    *(uint32_t*)proc->esp = (uint32_t)process_prelaunch; // return address after context switch
    
    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process
    
    proc->id = pids++;
    proc->status = READY;
    proc->next = NULL;

    add_READY_process(proc);
}

void delete_process(process_t* proc)
{
    /* for now, we do not care about user space only stack */
    if(proc->stack)
        vfree(proc->stack);

    if(proc->stack0)
        vfree(proc->stack0);

    // free address space

    kfree(proc);
}

void cleaner_task()
{
    unlock_sheduler();  // because it's the first time

    process_t* dead_task;
    while(true)
    {
        if(first_DEAD_process != NULL)
        {
            lock_sheduler();
            
            dead_task = first_DEAD_process;

            if(first_DEAD_process == last_DEAD_process)
                last_DEAD_process = NULL;
            
            first_DEAD_process = first_DEAD_process->next;
            
            log_debug("cleaner", "cleaning 0x%x", dead_task);

            unlock_sheduler();

            delete_process(dead_task);
            continue;
        }

        block_current_task();
        yield();
    }
}

void initialize_multitasking()
{
    // create the first process which is the idle process
    idle = kmalloc(sizeof(process_t));

    idle->stack = NULL;     // no need to create a new stack because initially we already have one
    idle->stack0 = NULL;    // because it's a kernel process

    idle->esp = NULL;       // this will be filled automatically when a context switch occurs

    idle->page_directory = getPDBR();
    idle->id = pids++;
    idle->next = NULL;

    current_process = idle;
    current_process->status = RUNNING;

    // create the cleaner process
    
    cleaner_process = kmalloc(sizeof(process_t));

    cleaner_process->stack = vmalloc(1);
    cleaner_process->stack0 = NULL;

    cleaner_process->esp = cleaner_process->stack + 0x1000 - 4;
    *(uint32_t*)cleaner_process->esp = (uint32_t)cleaner_task; // return address after context switch

    cleaner_process->esp -= (4 * 5);   // pushed register
    *(uint32_t*)cleaner_process->esp = 0x202;       // default eflags for the new process

    cleaner_process->page_directory = getPDBR();
    cleaner_process->id = pids++;
    cleaner_process->status = BLOCKED;
    cleaner_process->next = NULL;
}

void terminate_task()
{
    lock_sheduler();

    add_DEAD_process(current_process);

    if(cleaner_process->status == BLOCKED)
        unblock_task(cleaner_process);

    unlock_sheduler();
    yield();
}

// sleeping list
typedef struct sleep_process
{
    process_t* proc;
    uint64_t wake_time;
	struct sleep_process *next;
}__attribute__((packed)) sleep_process_t;

sleep_process_t* sleep_list = NULL;

void task_sleep(uint32_t ms)
{
    lock_sheduler();

    sleep_process_t* sleep_proc = kmalloc(sizeof(sleep_process_t));

    sleep_proc->wake_time = getTickCount() + ms;
    sleep_proc->proc = current_process;
    sleep_proc->next = NULL;

    block_current_task();
    
    if(sleep_list == NULL)
        sleep_list = sleep_proc;
    else
    {
        sleep_process_t* current = sleep_list;

        if(current->wake_time > sleep_proc->wake_time)
        {
            sleep_proc->next = sleep_list;
            sleep_list = sleep_proc;

            goto End;
        }

        while (current->next != NULL)
        {
            if(current->next->wake_time > sleep_proc->wake_time)
            {
                sleep_proc->next = current->next;
                current->next = sleep_proc;

                goto End;
            }

            current = current->next;
        }

        current->next = sleep_proc;
    }

End:
    unlock_sheduler();
    yield();
}

void task_wakeUp()
{
    lock_sheduler();

    if(sleep_list == NULL)
        goto End;

    sleep_process_t* current;

    while(sleep_list != NULL && sleep_list->wake_time <= getTickCount())
    {
        unblock_task(sleep_list->proc);

        current = sleep_list;
        sleep_list = sleep_list->next;
        kfree(current);
    }

End:
    unlock_sheduler();
}

mutex_t* create_mutex()
{
    mutex_t* mut = kmalloc(sizeof(mutex_t));

    mut->locked = false;
    mut->first_waiting_list = NULL;
    mut->last_waiting_list = NULL;

    return mut;
}

void destroy_mutex(mutex_t* mut)
{
    kfree(mut);
}

void acquire_mutex(mutex_t* mut)
{
    if(mut->locked)
    {
        lock_sheduler();
        current_process->next = NULL;

        if(!mut->first_waiting_list)
            mut->first_waiting_list = current_process;
    
        if(mut->last_waiting_list)
            mut->last_waiting_list->next = current_process;

        mut->last_waiting_list = current_process;
        mut->last_waiting_list->next = NULL;

        block_current_task();
        unlock_sheduler();
        yield();
    }
    else
    {
        mut->locked = true;
    }
}

void release_mutex(mutex_t* mut)
{
    if(mut->first_waiting_list != NULL)
    {
        lock_sheduler();

        process_t* released = mut->first_waiting_list;

        if(mut->first_waiting_list == mut->last_waiting_list)
            mut->last_waiting_list = mut->last_waiting_list->next; // null
        
        mut->first_waiting_list = mut->first_waiting_list->next;

        unblock_task(released);
        unlock_sheduler();
    }
    else
    {
        mut->locked = false;
    }
}