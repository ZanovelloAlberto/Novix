/*
 * File: kernel.c
 *
 * Description:
 *      This file implements the main functions for our OS.
 *
 * Author: Novice
 * last modification: 11/9th/2024
 */


#include "vga.h"

void kmain(void);

void kmain(void){
    reset();
    print("hello in protected mode !");
}