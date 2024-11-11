/*
 * File: vga.h
 *
 * Description: header's function file for vga.c
 *
 * Author: Novice
 * last modification: 11/9th/2024
 */

#pragma once
#include "stdint.h"

#define COLOR8_BLACK 0
#define COLOR8_LIGHT_GREY 7

#define WIDTH 80
#define HEIGHT 25

void print(const char* s);
void scrollUp();
void newLine();
void reset();
void updateCursor();
