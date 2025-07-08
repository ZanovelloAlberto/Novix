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

#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum status {DEAD, RUNNING, READY, BLOCKED} status_t;

typedef struct process
{
    void* phys_pdbr_addr;
    void* virt_pdbr_addr;
	void* esp;
    void* stack;
    int id;
    bool user;
    status_t status;
	struct process *next;
}__attribute__((packed)) process_t;

typedef struct mutex
{
    bool locked;
    int locked_count;
    process_t* owner;
    process_t* first_waiting_list;
    process_t* last_waiting_list;
}mutex_t;

void yield();
void initialize_multitasking();
void create_process(void* task, bool is_user);
void __attribute__((cdecl)) context_switch(process_t* current, process_t* next);

void lock_sheduler();
void unlock_sheduler();

void terminate_task();

void sleep(uint32_t ms);
void wakeUp();

mutex_t* create_mutex();
void destroy_mutex(mutex_t* mut);
void acquire_mutex(mutex_t* mut);
void release_mutex(mutex_t* mut);