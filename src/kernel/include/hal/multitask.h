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

typedef enum status {DEAD, RUNNING, READY} status_t;

typedef struct process
{
    void* page_directory;
	void* esp;
    void* stack;
    void* stack0;
    int id;
    status_t status;
	struct process *next;
}__attribute__((packed)) process_t;

void yield();
void initialize_multitasking();
void create_kernel_process(void (*task)(void));
void __attribute__((cdecl)) context_switch(process_t* current, process_t* next);