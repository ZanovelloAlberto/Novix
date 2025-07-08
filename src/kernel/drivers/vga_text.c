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


#include <drivers/vga_text.h>
#include <hal/io.h>
#include <scheduler/multitask.h>

#include <stdbool.h>
#include <stddef.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define WIDTH 80
#define HEIGHT 25

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

mutex_t vga_lock = {
    .owner = NULL,
    .locked_count = 0,
    .locked = false,
    .last_waiting_list = NULL,
    .first_waiting_list = NULL
};

uint16_t column = 0;
uint16_t line = 0;
uint16_t* const vga = (uint16_t* const) 0xB8000;
const VGA_COLOR background = VGA_COLOR_BLACK;
const uint16_t defaultColor = (VGA_COLOR_LIGHT_GREY << 8) | (background << 12);

uint16_t currentColor = defaultColor;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

static void updateCursor();
static void newLine();
static void scrollUp();

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

/** updateCursor:
* update the cursor position with the line and column value
*/
static void updateCursor()
{
    unsigned short index = line * WIDTH + column;
    outb(0x3d4, 14);  //14 tells the framebuffer to expect the highest 8 bits of the position
    outb(0x3d5, (uint8_t) (index >> 8) & 0xff);

    outb(0x3d4, 15); //15 tells the framebuffer to expect the lowest 8 bits of the position
    outb(0x3d5, (uint8_t) index & 0x00ff);
}

/** newLine:
* handle the \n character
*/
static void newLine()
{
    if(line < HEIGHT - 1){
        line++;
        column = 0;
    }else{
        scrollUp();
        column = 0;
    }
}

/** scrollUp:
* scroll Up the screen after hitting the last line
*/
static void scrollUp()
{
    for(uint16_t y = 0; y < HEIGHT; y++){
        for(uint16_t x = 0; x < WIDTH; x++){
            vga[(y-1) * WIDTH + x] = vga[y * WIDTH + x];
        }
    }

    for(uint16_t x = 0; x < WIDTH; x++){
        vga[(HEIGHT - 1) * WIDTH + x] = ' ' | currentColor;
    }
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void VGA_setCurrentColor(VGA_COLOR foreground)
{
    acquire_mutex(&vga_lock);
    currentColor = (foreground << 8) | (background << 12);
    release_mutex(&vga_lock);
}

void VGA_setColorToDefault()
{
    acquire_mutex(&vga_lock);
    currentColor = defaultColor;
    release_mutex(&vga_lock);
}

void VGA_moveCursorTo(uint16_t new_line, uint16_t new_column)
{
    if(new_line >= HEIGHT || new_column >= WIDTH)
        return;

    acquire_mutex(&vga_lock);
    line = new_line;
    column = new_column;
    updateCursor();
    release_mutex(&vga_lock);
}

uint16_t VGA_getCurrentLine()
{
    return line;
}

uint16_t VGA_getCurrentColumn()
{
    return column;
}

/** clr:
* clear the screen with the defaut color and update the cursor
*/
void VGA_clr()
{
    acquire_mutex(&vga_lock);
    line = 0;
    column = 0;
    currentColor = defaultColor;

    for(uint16_t y = 0; y < HEIGHT; y++){
        for(uint16_t x = 0; x < WIDTH; x++){
            vga[y * WIDTH + x] = ' ' | defaultColor;
        }
    }

    updateCursor();
    release_mutex(&vga_lock);
}

/** VGA_puts:
* print a string to the screen
* @param s the string
*/
void VGA_puts(const char* s)
{
    acquire_mutex(&vga_lock);
    while(*s){
        VGA_putc(*s);
        s++;
        updateCursor();
    }
    release_mutex(&vga_lock);
}

void VGA_coloredPuts(const char* s, VGA_COLOR foreground)
{
    acquire_mutex(&vga_lock);
    VGA_setCurrentColor(foreground);
    VGA_puts(s);
    VGA_setColorToDefault();
    release_mutex(&vga_lock);
}

/** VGA_putc:
* print a charater to the screen
* @param c the character
*/
void VGA_putc(const char c)
{
    acquire_mutex(&vga_lock);
    switch(c){
        case '\n':
            newLine();
            break;
        case '\r':
            column = 0;
            break;
        case '\b':
            if(column == 0)
            {
                if(line > 0)
                {
                    line--;
                    column = WIDTH - 1;
                }
            }else{
                column--;
            }
            vga[line * WIDTH + (column)] = ' ' | currentColor;
            break;
        case '\t':
            if(column == WIDTH){
                newLine();
            }

            uint16_t tabLen = 4 - (column % 4);
            while(tabLen != 0){
                vga[line * WIDTH + (column++)] = ' ' | currentColor;
                tabLen--;
            }
            break;
        default:
            if(column == WIDTH){
                newLine();
            }

            vga[line * WIDTH + (column++)] = c | currentColor;
        break;
    }
    updateCursor();
    release_mutex(&vga_lock);
}