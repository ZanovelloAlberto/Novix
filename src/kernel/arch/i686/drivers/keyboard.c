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

uint8_t i686_keyboard_readStatusReg()
{
    return i686_inb(KYBRD_CTRL_STATUS_REG);
}

void i686_keyboard_sendCmd(uint8_t port, uint8_t cmd)
{
    while(1)
        if(!(i686_keyboard_readStatusReg() & KYBRD_CTRL_STATUS_IN_BUF))
            break;
    i686_outb(port, cmd);
}

uint8_t i686_keyboard_readOutputBuffer()
{
    while(1)
        if(i686_keyboard_readStatusReg() & KYBRD_CTRL_STATUS_OUT_BUF)
            break;
    return i686_inb(KYBRD_ENC_OUTPUT_BUF);
}

void i686_keyboard_updateLed(bool num, bool capslock, bool scroll)
{
    uint8_t data = 0;

    // set or clear the bit
    data = (scroll) ? (data | 1) : (data & 1);
    data = (num) ? (data | 2) : (data & 2);
    data = (capslock) ? (data | 4) : (data & 4);

    i686_keyboard_sendCmd(KYBRD_ENC_CMD_REG, 0xED);
    i686_keyboard_sendCmd(KYBRD_ENC_CMD_REG, data);
}

void i686_keyboard_updateScanCodeSet(KYBRD_SCANCODE_SET scanCodeSet)
{
    i686_keyboard_sendCmd(KYBRD_ENC_CMD_REG, 0xF0);
    i686_keyboard_sendCmd(KYBRD_ENC_CMD_REG, scanCodeSet);
}

void i686_keyboard_setTypematicMode(TYPEMATIC_DELAY delay, TYPEMATIC_RATE rate)
{
    i686_keyboard_sendCmd(KYBRD_ENC_CMD_REG, 0xF3);
    i686_keyboard_sendCmd(KYBRD_ENC_CMD_REG, rate | delay << 5);
}


bool i686_keyboard_selfTest()
{
    i686_keyboard_sendCmd(KYBRD_CTRL_CMD_REG, 0xAA);
    if(i686_keyboard_readOutputBuffer() == 0x55)
        return true;
    else
        return false;
}

bool i686_keyboard_interfaceTest()
{
    i686_keyboard_sendCmd(KYBRD_CTRL_CMD_REG, 0xAB);
    if(i686_keyboard_readOutputBuffer() == 0)
        return true;
    else
        return false;
}

void i686_keyboard_disable()
{
    i686_keyboard_sendCmd(KYBRD_CTRL_CMD_REG, 0xAD);
    g_keyboard_stateDisabled = true;
}

void i686_keyboard_enable()
{
    i686_keyboard_sendCmd(KYBRD_CTRL_CMD_REG, 0xAE);
    g_keyboard_stateDisabled = false;
}

void i686_keyboard_discardLastKey()
{
    g_scancode = NULL_KEY;
}

KEYCODE i686_keyboard_getLastKey()
{
    return g_scancode;
}

void i686_keyboard_interruptHandler(Registers* regs)
{
    g_scancode = i686_keyboard_readOutputBuffer();

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
                i686_keyboard_updateLed(g_numLockOn, g_capsLockOn, g_scrollOn);
                break;
            case NUMLOCK_PRESSED:
                g_numLockOn ^= true;
                i686_keyboard_updateLed(g_numLockOn, g_capsLockOn, g_scrollOn);
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

void i686_keyboard_initialize()
{
    printf("initializing Keyboard...\n\r");

    i686_disableInterrupts();
    i686_keyboard_enable(); // just in case !

    if(!i686_keyboard_selfTest() || !i686_keyboard_interfaceTest())
    {
        printf("keyboard initialization failed");
        i686_keyboard_disable();
        return;
    }

    i686_keyboard_updateScanCodeSet(SCANCODE_SET_2);
    i686_keyboard_setTypematicMode(TYPEMATIC_DELAY_1000, TYPEMATIC_RATE_2PER_SEC);
    
    i686_IRQ_registerNewHandler(1, (IRQHandler)i686_keyboard_interruptHandler);
    i686_enableInterrupts();

    printf("Done !\n\r");
}

char i686_keyboard_scanToAscii(uint8_t scancode)
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