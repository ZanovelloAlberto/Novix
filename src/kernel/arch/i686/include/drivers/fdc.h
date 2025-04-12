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
#include <hal/irq.h>

typedef enum {
    DATA_RATE_500KPS    = 0x00,
    DATA_RATE_250KPS    = 0x02,
    DATA_RATE_300KPS    = 0x01,
    DATA_RATE_1MPS      = 0x03,
}DATA_RATE;

void i686_fdcInterruptHandler(Registers regs);
bool i686_fdcWaitIrq();
void i686_fdcInitializeDma(uint32_t phys_buffer, uint32_t count);
void i686_fdcDmaRead();
void i686_fdcDmaWrite();
void i686_fdcWriteDor(uint8_t flags);
uint8_t i686_fdcReadMsr();
void i686_fdcDisableController();
void i686_fdcEnableController();
void i686_fdcSendCommand(uint8_t cmd);
uint8_t i686_fdcReadData();
void i686_fdcConfigureDrive(uint32_t step_rate, uint32_t head_load_time, uint32_t head_unload_time, bool dma);
void i686_fdcSelectDataRate(DATA_RATE rate);
void i686_fdcCheckInterruptStatus(uint32_t* st0, uint32_t* cyl);
void i686_fdcControlMotor(bool is_on);
bool i686_fdcCalibrate();
void i686_fdcSetCurrentDrive(uint8_t drive);
void i686_fdcResetController();
void i686_fdcSectorRead(uint8_t head, uint8_t track, uint8_t sector, uint32_t phys_buffer);
bool i686_fdcSeek(uint32_t cyl, uint32_t head);
void i686_fdcInitialize();