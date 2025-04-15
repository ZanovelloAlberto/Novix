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

#include <stdbool.h>
#include <stdio.h>
#include <hal/irq.h>
#include <hal/io.h>
#include <drivers/keyboard.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

typedef enum {
    KYBRD_ENC_OUTPUT_BUF    = 0x60,
    KYBRD_ENC_CMD_REG       = 0x60,
}KYBRD_ENCODER_IO;

typedef enum {
    KYBRD_CTRL_STATUS_REG   = 0x64,
    KYBRD_CTRL_CMD_REG      = 0x64,
}KYBRD_CTRL_IO;

typedef enum {
    KYBRD_CTRL_STATUS_OUT_BUF   = 0x01, //00000001
    KYBRD_CTRL_STATUS_IN_BUF    = 0x02, //00000010
    KYBRD_CTRL_STATUS_SYSTEM    = 0x04, //00000100
    KYBRD_CTRL_STATUS_CMD_DATA  = 0x08, //00001000
    KYBRD_CTRL_STATUS_LOCKED    = 0x10, //00010000
    KYBRD_CTRL_STATUS_AUX_BUF   = 0x20, //00100000
    KYBRD_CTRL_STATUS_TIMEOUT   = 0x40, //01000000
    KYBRD_CTRL_STATUS_PARITY    = 0x80, //10000000
}KYBRD_CTRL_STATUS;

typedef enum {
    SCANCODE_SET_1  = 0x01,
    SCANCODE_SET_2  = 0x02,
    SCANCODE_SET_3  = 0x03,
}KYBRD_SCANCODE_SET;

typedef enum{
    TYPEMATIC_DELAY_250     = 0x0,
    TYPEMATIC_DELAY_500     = 0x1,
    TYPEMATIC_DELAY_750     = 0x2,
    TYPEMATIC_DELAY_1000    = 0x3,
}TYPEMATIC_DELAY;

typedef enum{
    TYPEMATIC_RATE_30PER_SEC    = 0X00,
    TYPEMATIC_RATE_26PER_SEC    = 0x02,
    TYPEMATIC_RATE_24PER_SEC    = 0x02,
    TYPEMATIC_RATE_10PER_SEC    = 0x0F,
    TYPEMATIC_RATE_2PER_SEC     = 0x1F,
}TYPEMATIC_RATE;


//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

bool g_keyboard_stateDisabled = false;
bool g_shiftPressed = false;
bool g_capsLockOn = false;
bool g_numLockOn = false;
bool g_scrollOn = false;
bool g_extended = false;
KEYCODE g_scancode = NULL_KEY;

static char asciiTable[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

static char shiftTable[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

uint8_t KEYBOARD_readStatusReg();
void KEYBOARD_sendCmd(uint8_t port, uint8_t cmd);
uint8_t KEYBOARD_readOutputBuffer();
void KEYBOARD_updateLed(bool num, bool capslock, bool scroll);
void KEYBOARD_updateScanCodeSet(KYBRD_SCANCODE_SET scanCodeSet);
void KEYBOARD_setTypematicMode(TYPEMATIC_DELAY delay, TYPEMATIC_RATE rate);
bool KEYBOARD_selfTest();
bool KEYBOARD_interfaceTest();
void KEYBOARD_interruptHandler(Registers* regs);

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

uint8_t KEYBOARD_readStatusReg()
{
    return inb(KYBRD_CTRL_STATUS_REG);
}

void KEYBOARD_sendCmd(uint8_t port, uint8_t cmd)
{
    while(1)
        if(!(KEYBOARD_readStatusReg() & KYBRD_CTRL_STATUS_IN_BUF))
            break;
    outb(port, cmd);
}

uint8_t KEYBOARD_readOutputBuffer()
{
    while(1)
        if(KEYBOARD_readStatusReg() & KYBRD_CTRL_STATUS_OUT_BUF)
            break;
    return inb(KYBRD_ENC_OUTPUT_BUF);
}

void KEYBOARD_updateLed(bool num, bool capslock, bool scroll)
{
    uint8_t data = 0;

    // set or clear the bit
    data = (scroll) ? (data | 1) : (data & 1);
    data = (num) ? (data | 2) : (data & 2);
    data = (capslock) ? (data | 4) : (data & 4);

    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, 0xED);
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, data);
}

void KEYBOARD_updateScanCodeSet(KYBRD_SCANCODE_SET scanCodeSet)
{
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, 0xF0);
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, scanCodeSet);
}

void KEYBOARD_setTypematicMode(TYPEMATIC_DELAY delay, TYPEMATIC_RATE rate)
{
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, 0xF3);
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, rate | delay << 5);
}


bool KEYBOARD_selfTest()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAA);
    if(KEYBOARD_readOutputBuffer() == 0x55)
        return true;
    else
        return false;
}

bool KEYBOARD_interfaceTest()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAB);
    if(KEYBOARD_readOutputBuffer() == 0)
        return true;
    else
        return false;
}

void KEYBOARD_interruptHandler(Registers* regs)
{
    g_scancode = KEYBOARD_readOutputBuffer();

    if (g_scancode == 0xE0)
    {
        g_extended = true; // the next key is extended
        return;
    }

    if (g_scancode == 0xE1)
    {
        // handle pause key ...
        return;
    }

    if (g_scancode >= 0x80)
    {
        // BREAK CODE (key released)
        KEYCODE keyReleased = g_scancode & 0x7F; // get rid of bit 7
        
        switch (g_scancode)
        {
        case LSHIFT_RELEASED:
            g_shiftPressed = false;
            break;
        
        case RSHIFT_RELEASED:
            g_shiftPressed = false;
            break;
        
        default:
            break;
        }
    }
    else
    {
        // MAKE CODE (key pressed)
        if (g_extended)
        {
            // handling special keys like arrows etc.
            g_extended = false;
        }
        else
        {
            switch (g_scancode)
            {
            case CAPSLOCK_PRESSED:
                g_capsLockOn ^= true;
                KEYBOARD_updateLed(g_numLockOn, g_capsLockOn, g_scrollOn);
                break;
            case NUMLOCK_PRESSED:
                g_numLockOn ^= true;
                KEYBOARD_updateLed(g_numLockOn, g_capsLockOn, g_scrollOn);
                break;

            case LSHIFT_PRESSED:
                g_shiftPressed = true;
                break;
            
            case RSHIFT_PRESSED:
                g_shiftPressed = true;
                break;
            
            default:
                break;
            }
        }
    }
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void KEYBOARD_disable()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAD);
    g_keyboard_stateDisabled = true;
}

void KEYBOARD_enable()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAE);
    g_keyboard_stateDisabled = false;
}

void KEYBOARD_discardLastKey()
{
    g_scancode = NULL_KEY;
}

KEYCODE KEYBOARD_getLastKey()
{
    return g_scancode;
}

void KEYBOARD_initialize()
{
    printf("initializing Keyboard...\n\r");

    disableInterrupts();
    KEYBOARD_enable(); // just in case !

    if(!KEYBOARD_selfTest() || !KEYBOARD_interfaceTest())
    {
        printf("keyboard initialization failed");
        KEYBOARD_disable();
        return;
    }

    KEYBOARD_updateScanCodeSet(SCANCODE_SET_2);
    KEYBOARD_setTypematicMode(TYPEMATIC_DELAY_1000, TYPEMATIC_RATE_2PER_SEC);
    
    IRQ_registerNewHandler(1, (IRQHandler)KEYBOARD_interruptHandler);
    enableInterrupts();

    printf("Done !\n\r");
}

char KEYBOARD_scanToAscii(uint8_t scancode)
{
    if (scancode > 127) 
        return NULL_KEY;

    if(g_shiftPressed && g_capsLockOn)
        return asciiTable[scancode];
    if (g_shiftPressed || g_capsLockOn)
        return shiftTable[scancode];
    else
        return asciiTable[scancode];
}