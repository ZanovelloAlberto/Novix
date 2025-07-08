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
#include <vfs/vfs.h>
#include <scheduler/usermode.h>
#include <scheduler/multitask.h>

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
        if(next->user)    // if it's a usermode process
            TSS_setKernelStack((uint32_t)next->stack + 0x1000);

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

void spawn_process()
{
    unlock_sheduler();

    if(current_process->user)
    {
        // assume we have received a path string of a file
        char* path = (char*)(*(uint32_t*)(current_process->esp + (4 * 6)));
        int fd1 = VFS_open(path, VFS_O_RDWR);

        if(fd1 < 0)
        {
            log_err("spawn_process", "failed to open the file");
            terminate_task();
            return;
        }

        VIRTMEM_mapPage ((void*)0x400000, false);

        VFS_read(fd1, (void*)0x400000, 4095);
        VFS_close(fd1);

        switch_to_usermode(0x400000+4095, 0x400000);
    }
    else
    {
        // assume we have received a pointer to the entry point of the process
        void (*task)(void) = (void*)(*(uint32_t*)(current_process->esp + (4 * 6)));
        task();
    }
}

void create_process(void* task, bool is_user)
{
    process_t* proc = kmalloc(sizeof(process_t));

    uint32_t* pdbr = getPDBR();
    proc->virt_pdbr_addr = VIRTMEM_createAddressSpace();
    proc->phys_pdbr_addr = VIRTMEM_getPhysAddr(proc->virt_pdbr_addr);

    proc->stack = vmalloc(1);

    proc->esp = proc->stack + 0x1000 - 4;
    *(uint32_t*)proc->esp = (uint32_t)task; // argument for spawn_process

    proc->esp -= 4;
    *(uint32_t*)proc->esp = (uint32_t)spawn_process; // return address after context switch
    
    proc->esp -= (4 * 5);   // pushed register
    *(uint32_t*)proc->esp = 0x202;       // default eflags for the new process
    
    proc->id = pids++;
    proc->user = is_user;
    proc->status = READY;
    proc->next = NULL;

    add_READY_process(proc);
}

void delete_process(process_t* proc)
{
    // free address space
    VIRTMEM_destroyAddressSpace(proc->virt_pdbr_addr);

    if(proc->stack)
        vfree(proc->stack);

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

    idle->esp = NULL;       // this will be filled automatically when a context switch occurs

    idle->phys_pdbr_addr = getPDBR();
    idle->virt_pdbr_addr = NULL;
    idle->id = pids++;
    idle->user = false;
    idle->next = NULL;

    current_process = idle;
    current_process->status = RUNNING;

    // create the cleaner process
    
    cleaner_process = kmalloc(sizeof(process_t));

    cleaner_process->stack = vmalloc(1);

    cleaner_process->esp = cleaner_process->stack + 0x1000 - 4;
    *(uint32_t*)cleaner_process->esp = (uint32_t)cleaner_task; // return address after context switch

    cleaner_process->esp -= (4 * 5);   // pushed register
    *(uint32_t*)cleaner_process->esp = 0x202;       // default eflags for the new process

    cleaner_process->phys_pdbr_addr = getPDBR();
    cleaner_process->virt_pdbr_addr = NULL;
    cleaner_process->id = pids++;
    cleaner_process->user = false;
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

void sleep(uint32_t ms)
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

void wakeUp()
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
    mut->locked_count = 0;
    mut->owner = NULL;
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
        if(mut->owner == current_process)
        {
            mut->locked_count++;    // it's okay we can acquire a mutex multiple time
            return; 
        }
        
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
        mut->owner = current_process;
    }
}

void release_mutex(mutex_t* mut)
{
    if(mut->owner != current_process)
    {
        log_err("mutex", "Process %d tried to release mutex it doesn't own!", current_process->id);
        return;
    }

    if(mut->locked_count != 0)
    {
        mut->locked_count--;
        return;
    }

    if(mut->first_waiting_list != NULL)
    {
        lock_sheduler();

        process_t* released = mut->first_waiting_list;

        if(mut->first_waiting_list == mut->last_waiting_list)
            mut->last_waiting_list = mut->last_waiting_list->next; // null
        
        mut->first_waiting_list = mut->first_waiting_list->next;

        mut->owner = released;  // new owner

        unblock_task(released);
        unlock_sheduler();
    }
    else
    {
        mut->locked = false;
        mut->owner = NULL;
    }
}