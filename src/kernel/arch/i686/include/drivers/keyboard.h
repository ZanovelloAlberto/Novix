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
#include <stdint.h>

typedef enum {
    NULL_KEY = 0,
    
    // Letters
    A_PRESSED = 0x1E, A_RELEASED = 0x9E,
    B_PRESSED = 0x30, B_RELEASED = 0xB0,
    C_PRESSED = 0x2E, C_RELEASED = 0xAE,
    D_PRESSED = 0x20, D_RELEASED = 0xA0,
    E_PRESSED = 0x12, E_RELEASED = 0x92,
    F_PRESSED = 0x21, F_RELEASED = 0xA1,
    G_PRESSED = 0x22, G_RELEASED = 0xA2,
    H_PRESSED = 0x23, H_RELEASED = 0xA3,
    I_PRESSED = 0x17, I_RELEASED = 0x97,
    J_PRESSED = 0x24, J_RELEASED = 0xA4,
    K_PRESSED = 0x25, K_RELEASED = 0xA5,
    L_PRESSED = 0x26, L_RELEASED = 0xA6,
    M_PRESSED = 0x32, M_RELEASED = 0xB2,
    N_PRESSED = 0x31, N_RELEASED = 0xB1,
    O_PRESSED = 0x18, O_RELEASED = 0x98,
    P_PRESSED = 0x19, P_RELEASED = 0x99,
    Q_PRESSED = 0x10, Q_RELEASED = 0x90,
    R_PRESSED = 0x13, R_RELEASED = 0x93,
    S_PRESSED = 0x1F, S_RELEASED = 0x9F,
    T_PRESSED = 0x14, T_RELEASED = 0x94,
    U_PRESSED = 0x16, U_RELEASED = 0x96,
    V_PRESSED = 0x2F, V_RELEASED = 0xAF,
    W_PRESSED = 0x11, W_RELEASED = 0x91,
    X_PRESSED = 0x2D, X_RELEASED = 0xAD,
    Y_PRESSED = 0x15, Y_RELEASED = 0x95,
    Z_PRESSED = 0x2C, Z_RELEASED = 0xAC,
    
    // Numbers
    ZERO_PRESSED = 0x0B, ZERO_RELEASED = 0x8B,
    ONE_PRESSED = 0x02, ONE_RELEASED = 0x82,
    TWO_PRESSED = 0x03, TWO_RELEASED = 0x83,
    THREE_PRESSED = 0x04, THREE_RELEASED = 0x84,
    FOUR_PRESSED = 0x05, FOUR_RELEASED = 0x85,
    FIVE_PRESSED = 0x06, FIVE_RELEASED = 0x86,
    SIX_PRESSED = 0x07, SIX_RELEASED = 0x87,
    SEVEN_PRESSED = 0x08, SEVEN_RELEASED = 0x88,
    EIGHT_PRESSED = 0x09, EIGHT_RELEASED = 0x89,
    NINE_PRESSED = 0x0A, NINE_RELEASED = 0x8A,
    
    // Special Keys
    BACKSPACE_PRESSED = 0x0E, BACKSPACE_RELEASED = 0x8E,
    SPACE_PRESSED = 0x39, SPACE_RELEASED = 0xB9,
    ENTER_PRESSED = 0x1C, ENTER_RELEASED = 0x9C,
    ESC_PRESSED = 0x01, ESC_RELEASED = 0x81,
    TAB_PRESSED = 0x0F, TAB_RELEASED = 0x8F,
    CAPSLOCK_PRESSED = 0x3A, CAPSLOCK_RELEASED = 0xBA,
    
    // Shift, Control, Alt
    LSHIFT_PRESSED = 0x2A, LSHIFT_RELEASED = 0xAA,
    RSHIFT_PRESSED = 0x36, RSHIFT_RELEASED = 0xB6,
    LCTRL_PRESSED = 0x1D, LCTRL_RELEASED = 0x9D,
    RCTRL_PRESSED = 0xE01D, RCTRL_RELEASED = 0xE09D,
    LALT_PRESSED = 0x38, LALT_RELEASED = 0xB8,
    RALT_PRESSED = 0xE038, RALT_RELEASED = 0xE0B8,
    
    // Function Keys
    F1_PRESSED = 0x3B, F1_RELEASED = 0xBB,
    F2_PRESSED = 0x3C, F2_RELEASED = 0xBC,
    F3_PRESSED = 0x3D, F3_RELEASED = 0xBD,
    F4_PRESSED = 0x3E, F4_RELEASED = 0xBE,
    F5_PRESSED = 0x3F, F5_RELEASED = 0xBF,
    F6_PRESSED = 0x40, F6_RELEASED = 0xC0,
    F7_PRESSED = 0x41, F7_RELEASED = 0xC1,
    F8_PRESSED = 0x42, F8_RELEASED = 0xC2,
    F9_PRESSED = 0x43, F9_RELEASED = 0xC3,
    F10_PRESSED = 0x44, F10_RELEASED = 0xC4,
    F11_PRESSED = 0x57, F11_RELEASED = 0xD7,
    F12_PRESSED = 0x58, F12_RELEASED = 0xD8,
    
    // Navigation Keys
    INSERT_PRESSED = 0xE052, INSERT_RELEASED = 0xE0D2,
    DELETE_PRESSED = 0xE053, DELETE_RELEASED = 0xE0D3,
    HOME_PRESSED = 0xE047, HOME_RELEASED = 0xE097,
    END_PRESSED = 0xE04F, END_RELEASED = 0xE0CF,
    PAGEUP_PRESSED = 0xE049, PAGEUP_RELEASED = 0xE0C9,
    PAGEDOWN_PRESSED = 0xE051, PAGEDOWN_RELEASED = 0xE0D1,
    
    // Arrow Keys
    UP_PRESSED = 0xE048, UP_RELEASED = 0xE0C8,
    DOWN_PRESSED = 0xE050, DOWN_RELEASED = 0xE0D0,
    LEFT_PRESSED = 0xE04B, LEFT_RELEASED = 0xE0CB,
    RIGHT_PRESSED = 0xE04D, RIGHT_RELEASED = 0xE0CD,
    
    // Keypad Keys
    NUMLOCK_PRESSED = 0x45, NUMLOCK_RELEASED = 0xC5,
    KP_DIV_PRESSED = 0xE035, KP_DIV_RELEASED = 0xE0B5,
    KP_MUL_PRESSED = 0x37, KP_MUL_RELEASED = 0xB7,
    KP_SUB_PRESSED = 0x4A, KP_SUB_RELEASED = 0xCA,
    KP_ADD_PRESSED = 0x4E, KP_ADD_RELEASED = 0xCE,
    KP_ENTER_PRESSED = 0xE01C, KP_ENTER_RELEASED = 0xE09C
}KEYCODE;

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

typedef struct {
    uint8_t scancode;
    char ascii;
    char ascii_shift;
} KeyMapEntry;

uint8_t i686_keyboard_readStatusReg();
void i686_keyboard_sendCmd(uint8_t port, uint8_t cmd);
uint8_t i686_keyboard_readOutputBuffer();
void i686_keyboard_updateLed(bool num, bool capslock, bool scroll);
void i686_keyboard_updateScanCodeSet(KYBRD_SCANCODE_SET scanCodeSet);
void i686_keyboard_setTypematicMode(TYPEMATIC_DELAY delay, TYPEMATIC_RATE rate);
bool i686_keyboard_selfTest();
bool i686_keyboard_interfaceTest();
void i686_keyboard_disable();
void i686_keyboard_enable();
void i686_keyboard_discardLastKey();
KEYCODE i686_keyboard_getLastKey();
void i686_keyboard_initialize();
char i686_keyboard_scanToAscii(uint8_t scancode);