/*
 * File: vga.c
 *
 * Description:
 *      This file implements a set of functions to control VGA graphics display.
 *      It includes functions to update the cursor and perform other related operations.
 *
 * Author: Novice
 * last modification: 11/9th/2024
 */


#include "vga.h"
#include "util.h"

uint16_t column = 0;
uint16_t line = 0;
uint16_t* const vga = (uint16_t* const) 0xB8000;
const uint16_t defaultColor = (COLOR8_LIGHT_GREY << 8) | (COLOR8_BLACK << 12);

uint16_t currentColor = defaultColor;

/** updateCursor:
* update the cursor position with the line and column value
*/
void updateCursor(){
    unsigned short index = (line * WIDTH) + column;
    outb(0x3d4, 14);  //14 tells the framebuffer to expect the highest 8 bits of the position
    outb(0x3d5, (unsigned char) index >> 8);

    outb(0x3d4, 15); //15 tells the framebuffer to expect the lowest 8 bits of the position
    outb(0x3d5, (unsigned char) index & 0x00ff);
}

/** reset:
* reset the screen with the defaut color and update the cursor
*/
void reset(){
    line = 0;
    column = 0;
    currentColor = defaultColor;

    for(uint16_t y = 0; y < HEIGHT; y++){
        for(uint16_t x = 0; x < WIDTH; x++){
            vga[y * WIDTH + x] = ' ' | defaultColor;
        }
    }

    updateCursor();
}

/** newLine:
* handle the \n character
*/
void newLine(){
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
void scrollUp(){
    for(uint16_t y = 0; y < HEIGHT; y++){
        for(uint16_t x = 0; x < WIDTH; x++){
            vga[(y-1) * WIDTH + x] = vga[y * WIDTH + x];
        }
    }

    for(uint16_t x = 0; x < WIDTH; x++){
        vga[(HEIGHT - 1) * WIDTH + x] = ' ' | currentColor;
    }
}

/** print:
* print a charater to the screen
* @param s the character
*/
void print(const char* s){
    while(*s){
        switch(*s){
            case '\n':
                newLine();
                break;
            case '\r':
                column = 0;
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

                vga[line * WIDTH + (column++)] = *s | currentColor;
            break;
        }
        s++;
        updateCursor();
    }
}