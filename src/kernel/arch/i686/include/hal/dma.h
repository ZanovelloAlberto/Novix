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

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DMA_MODE_MASK_SELECT_0 = 0x00,
    DMA_MODE_MASK_SELECT_1 = 0x01,
    DMA_MODE_MASK_SELECT_2 = 0x02,
    DMA_MODE_MASK_SELECT_3 = 0x03,

    DMA_MODE_MASK_SELF_TEST         = 0x00,
    DMA_MODE_MASK_READ_TRANSFER     = 0x04,
    DMA_MODE_MASK_WRITE_TRANSFER    = 0x08,

    DMA_MODE_MASK_AUTO  = 0x10,
    DMA_MODE_MASK_IDEC  = 0x20,

    DMA_MODE_MASK_TRANSFER_ON_DEMAND    = 0x00,
    DMA_MODE_MASK_TRANSFER_SINGLE       = 0x40,
    DMA_MODE_MASK_TRANSFER_BLOCK        = 0x80,
    DMA_MODE_MASK_TRANSFER_CASCADE      = 0xC0,
}DMA_MODE_MASK;

void i686_dmaEnable();
void i686_dmaDisable();
void i686_dmaSetChannelAddr(uint8_t channel, uint32_t phys_addr);
void i686_dmaSetChannelCounter(uint8_t channel, uint16_t count);
void i686_dmaMaskChannel(uint16_t channel);
void i686_dmaUnmaskChannel(uint16_t channel);
void i686_dmaSetMode(uint16_t channel, uint8_t mode);
void i686_dmaResetFlipFlop(bool is_master_dma);
void i686_dmaReset(bool is_master_dma);