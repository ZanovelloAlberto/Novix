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

#include <hal/dma.h>
#include <hal/io.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

typedef enum {
    MASTER_DMA_PORT_STATUS_REG          = 0xD0,
    MASTER_DMA_PORT_COMMAND_REG         = 0xD0,
    MASTER_DMA_PORT_REQUEST_REG         = 0xD2,
    MASTER_DMA_PORT_SINGLEMASK_REG      = 0xD4,
    MASTER_DMA_PORT_MODE_REG            = 0xD6,
    MASTER_DMA_PORT_CLEARBYTE_FLIP_FLOP = 0xD8,
    MASTER_DMA_PORT_INTERMEDIATE_REG    = 0xDA,
    MASTER_DMA_PORT_MASTER_CLEAR        = 0xDA,
    MASTER_DMA_PORT_CLEARMASK_REG       = 0xDC,
    MASTER_DMA_PORT_WRITEMASK_REG       = 0xDE,
}MASTER_DMA_PORT;

typedef enum {
    SLAVE_DMA_PORT_STATUS_REG          = 0x08,
    SLAVE_DMA_PORT_COMMAND_REG         = 0x08,
    SLAVE_DMA_PORT_REQUEST_REG         = 0x09,
    SLAVE_DMA_PORT_SINGLEMASK_REG      = 0x0A,
    SLAVE_DMA_PORT_MODE_REG            = 0x0B,
    SLAVE_DMA_PORT_CLEARBYTE_FLIP_FLOP = 0x0C,
    SLAVE_DMA_PORT_INTERMEDIATE_REG    = 0x0D,
    SLAVE_DMA_PORT_MASTER_CLEAR        = 0x0D,
    SLAVE_DMA_PORT_CLEARMASK_REG       = 0x0E,
    SLAVE_DMA_PORT_WRITEMASK_REG       = 0x0F,
}SLAVE_DMA_PORT;

typedef enum {
    MASTER_DMA_CHANNEL_ADDRESS_4 = 0xC0,
    MASTER_DMA_CHANNEL_COUNTER_4 = 0xC2,
    MASTER_DMA_CHANNEL_ADDRESS_5 = 0xC4,
    MASTER_DMA_CHANNEL_COUNTER_5 = 0xC6,
    MASTER_DMA_CHANNEL_ADDRESS_6 = 0xC8,
    MASTER_DMA_CHANNEL_COUNTER_6 = 0xCA,
    MASTER_DMA_CHANNEL_ADDRESS_7 = 0xCC,
    MASTER_DMA_CHANNEL_COUNTER_7 = 0xCE,
}MASTER_DMA_CHANNEL;

typedef enum {
    SLAVE_DMA_CHANNEL_ADDRESS_0 = 0x00,
    SLAVE_DMA_CHANNEL_COUNTER_0 = 0x01,
    SLAVE_DMA_CHANNEL_ADDRESS_1 = 0x02,
    SLAVE_DMA_CHANNEL_COUNTER_1 = 0x03,
    SLAVE_DMA_CHANNEL_ADDRESS_2 = 0x04,
    SLAVE_DMA_CHANNEL_COUNTER_2 = 0x05,
    SLAVE_DMA_CHANNEL_ADDRESS_3 = 0x06,
    SLAVE_DMA_CHANNEL_COUNTER_3 = 0x07,
}SLAVE_DMA_CHANNEL;

typedef enum {
    /* uses AT port mapping ...*/
    SLAVE_DMA_PAGEADDR_REG_CHANNEL_0 = 0x87,
    SLAVE_DMA_PAGEADDR_REG_CHANNEL_1 = 0x83,
    SLAVE_DMA_PAGEADDR_REG_CHANNEL_2 = 0x81,
    SLAVE_DMA_PAGEADDR_REG_CHANNEL_3 = 0x82,
}SLAVE_DMA_PAGEADDR_REG;

typedef enum {
    /* uses AT port mapping ...*/
    MASTER_DMA_PAGEADDR_REG_CHANNEL_4 = 0x8F, // Memory refresh / Slave Connect
    MASTER_DMA_PAGEADDR_REG_CHANNEL_5 = 0x8B,
    MASTER_DMA_PAGEADDR_REG_CHANNEL_6 = 0x89,
    MASTER_DMA_PAGEADDR_REG_CHANNEL_7 = 0x8A,
}MASTER_DMA_PAGEADDR_REG;

typedef enum {
    DMA_COMMAND_MASK_MEMTOMEM       = 0x01,
    DMA_COMMAND_MASK_CHAN0ADDRHOLD  = 0x02,
    DMA_COMMAND_MASK_ENABLE         = 0x04,
    DMA_COMMAND_MASK_TIMING         = 0x08,
    DMA_COMMAND_MASK_PRIORITY       = 0x10,
    DMA_COMMAND_MASK_WRITESELECTION = 0x20,
    DMA_COMMAND_MASK_DMAREQUEST     = 0x40,
    DMA_COMMAND_MASK_DACK           = 0x80,
}DMA_COMMAND_MASKS;

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void DMA_enable()
{
    outb(MASTER_DMA_PORT_COMMAND_REG, DMA_COMMAND_MASK_ENABLE);
}

void DMA_disable()
{
    outb(MASTER_DMA_PORT_COMMAND_REG, 0x0);
}

void DMA_resetFlipFlop(bool is_master_dma)
{
    if(is_master_dma)
        outb(MASTER_DMA_PORT_CLEARBYTE_FLIP_FLOP, 0x00);
    else
        outb(SLAVE_DMA_PORT_CLEARBYTE_FLIP_FLOP, 0x00);
}

void DMA_reset(bool is_master_dma)
{
    if(is_master_dma)
        outb(MASTER_DMA_PORT_MASTER_CLEAR, 0x00);
    else
        outb(SLAVE_DMA_PORT_MASTER_CLEAR, 0x00);
}

void DMA_maskChannel(uint16_t channel)
{
    if(channel >= 8)
        return;
    else if(channel < 4)
        outb(SLAVE_DMA_PORT_SINGLEMASK_REG, channel | 0b100);
    else
        outb(MASTER_DMA_PORT_SINGLEMASK_REG, (channel - 4) | 0b100);
}

void DMA_unmaskChannel(uint16_t channel)
{
    if(channel >= 8)
        return;
    else if(channel < 4)
        outb(SLAVE_DMA_PORT_SINGLEMASK_REG, channel | 0b000);
    else
        outb(MASTER_DMA_PORT_SINGLEMASK_REG, (channel - 4) | 0b000);
}

void DMA_setChannelAddr(uint8_t channel, uint32_t phys_addr)
{
    uint16_t channel_port;
    uint16_t page_port;

    switch (channel)
    {
    case 0:
        channel_port = SLAVE_DMA_CHANNEL_ADDRESS_0;
        page_port = SLAVE_DMA_PAGEADDR_REG_CHANNEL_0;
        break;
    case 1:
        channel_port = SLAVE_DMA_CHANNEL_ADDRESS_1;
        page_port = SLAVE_DMA_PAGEADDR_REG_CHANNEL_1;
        break;
    case 2:
        channel_port = SLAVE_DMA_CHANNEL_ADDRESS_2;
        page_port = SLAVE_DMA_PAGEADDR_REG_CHANNEL_2;
        break;
    case 3:
        channel_port = SLAVE_DMA_CHANNEL_ADDRESS_3;
        page_port = SLAVE_DMA_PAGEADDR_REG_CHANNEL_3;
        break;
    case 4:
        // slave dma connect unused !
        break;
    case 5:
        channel_port = MASTER_DMA_CHANNEL_ADDRESS_5;
        page_port = MASTER_DMA_PAGEADDR_REG_CHANNEL_5;
        break;
    case 6:
        channel_port = MASTER_DMA_CHANNEL_ADDRESS_6;
        page_port = MASTER_DMA_PAGEADDR_REG_CHANNEL_6;
        break;
    case 7:
        channel_port = MASTER_DMA_CHANNEL_ADDRESS_7;
        page_port = MASTER_DMA_PAGEADDR_REG_CHANNEL_7;
        break;
    default:
        return;
        break;
    }

    //set channel low base address
    outb(channel_port, (phys_addr) & 0xff);

    //set channel high base address
    outb(channel_port, (phys_addr >> 8) & 0xff);

    // set the extended page address registers
    outb(page_port, (phys_addr >> 16) & 0xff);
}

void DMA_setChannelCounter(uint8_t channel, uint16_t count)
{
    uint16_t channel_port;

    switch (channel)
    {
    case 0:
        channel_port = SLAVE_DMA_CHANNEL_COUNTER_0;
        break;
    case 1:
        channel_port = SLAVE_DMA_CHANNEL_COUNTER_1;
        break;
    case 2:
        channel_port = SLAVE_DMA_CHANNEL_COUNTER_2;
        break;
    case 3:
        channel_port = SLAVE_DMA_CHANNEL_COUNTER_3;
        break;
    case 4:
        // slave dma connect unused !
        break;
    case 5:
        channel_port = MASTER_DMA_CHANNEL_COUNTER_5;
        break;
    case 6:
        channel_port = MASTER_DMA_CHANNEL_COUNTER_6;
        break;
    case 7:
        channel_port = MASTER_DMA_CHANNEL_COUNTER_7;
        break;
    default:
        return;
        break;
    }

    //set channel low count
    outb(channel_port, (count) & 0xff);

    //set channel high count
    outb(channel_port, (count >> 8) & 0xff);
}

void DMA_setMode(uint16_t channel, uint8_t mode)
{
    if(channel >= 8)
        return;

    DMA_maskChannel(channel);
    outb(channel < 4 ? SLAVE_DMA_PORT_MODE_REG : MASTER_DMA_PORT_MODE_REG, channel < 4 ? channel | mode : (channel - 4) | mode);
    DMA_unmaskChannel(channel);
}
