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


//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void FDC_disableController();
void FDC_enableController();
void FDC_resetController();
void FDC_controlMotor(bool is_on);
bool FDC_calibrate();
void FDC_setCurrentDrive(uint8_t drive);
void FDC_sectorRead(uint8_t head, uint8_t track, uint8_t sector, uint32_t phys_buffer);
bool FDC_seek(uint32_t cyl, uint32_t head);
void FDC_initialize();