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

#include <stddef.h>
#include <hal/io.h>
#include <hal/pit.h>
#include <scheduler/multitask.h>

mutex_t e9_lock = {
    .owner = NULL,
    .locked_count = 0,
    .locked = false,
    .last_waiting_list = NULL,
    .first_waiting_list = NULL
};

void E9_putc(char c)
{
    if(is_multitaskingEnabled())
        acquire_mutex(&e9_lock);
    
    outb(0xE9, c);
    
    if(is_multitaskingEnabled())
        release_mutex(&e9_lock);
}