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

#include <drivers/fdc.h>
#include <hal/dma.h>
#include <hal/irq.h>
#include <hal/io.h>
#include <hal/physMemory_manager.h>
#include <stdio.h>

#define FDC_CHANNEL 2
#define FDC_BUFFER_BLOCKSIZE 64 / 4
#define FDC_SECTOR_PER_TRACK 18
#define FDC_HEAD 2

typedef enum {
    FDC_PORT_DOR    = 0x3f2,
    FDC_PORT_MSR    = 0x3f4,
    FDC_PORT_FIFO   = 0x3f5,
    FDC_PORT_CCR    = 0x3f7,
}FDC_PORT;

typedef enum {
    FDC_DOR_MASK_DRIVE0         = 0x00, //00000000 = here for completeness sake
    FDC_DOR_MASK_DRIVE1         = 0x01, //00000001
    FDC_DOR_MASK_DRIVE2         = 0x02, //00000010
    FDC_DOR_MASK_DRIVE3         = 0x03, //00000011
    FDC_DOR_MASK_RESET          = 0x04, //00000100
    FDC_DOR_MASK_DMA            = 0x08, //00001000
    FDC_DOR_MASK_DRIVE0_MOTOR   = 0x10, //00010000
    FDC_DOR_MASK_DRIVE1_MOTOR   = 0x20, //00100000
    FDC_DOR_MASK_DRIVE2_MOTOR   = 0x40, //01000000
    FDC_DOR_MASK_DRIVE3_MOTOR   = 0x80, //10000000
}FDC_DOR_MASK;

typedef enum {
    FDC_MSR_MASK_DRIVE1_POS_MODE    = 0x01, //00000001
    FDC_MSR_MASK_DRIVE2_POS_MODE    = 0x02, //00000010
    FDC_MSR_MASK_DRIVE3_POS_MODE    = 0x04, //00000100
    FDC_MSR_MASK_DRIVE4_POS_MODE    = 0x08, //00001000
    FDC_MSR_MASK_BUSY               = 0x10, //00010000
    FDC_MSR_MASK_DMA                = 0x20, //00100000
    FDC_MSR_MASK_DATAIO             = 0x40, //01000000
    FDC_MSR_MASK_DATAREG            = 0x80, //10000000
}FDC_MSR_MASK;

typedef enum {
    FDC_CMD_READ_TRACK      = 0x02,
    FDC_CMD_SPECIFY         = 0x03,
    FDC_CMD_CHECK_STAT      = 0x04,
    FDC_CMD_WRITE_SECT      = 0x05,
    FDC_CMD_READ_SECT       = 0x06,
    FDC_CMD_CALIBRATE       = 0x07,
    FDC_CMD_CHECK_INT       = 0x08,
    FDC_CMD_WRITE_DEL_S     = 0x09,
    FDC_CMD_READ_ID_S       = 0x0a,
    FDC_CMD_READ_DEL_S      = 0x0c,
    FDC_CMD_FORMAT_TRACK    = 0x0d,
    FDC_CMD_SEEK            = 0x0f,
}FDC_CMD;

typedef enum {
    FDC_CMD_EXT_SKIP        = 0x20, //00100000
    FDC_CMD_EXT_DENSITY     = 0x40, //01000000
    FDC_CMD_EXT_MULTITRACK  = 0x80 //10000000
}FDC_CMD_EXT;

typedef enum {
    FDC_GAP3_LENGTH_STD     = 42,
    FDC_GAP3_LENGTH_5_14    = 32,
    FDC_GAP3_LENGTH_3_5     = 27,
}FDC_GAP3_LENGTH;

typedef enum {
    FDC_SECTOR_SIZE_128  = 0,
    FDC_SECTOR_SIZE_256  = 1,
    FDC_SECTOR_SIZE_512  = 2,
    FDC_SECTOR_SIZE_1024 = 4,
}FDC_SECTOR_SIZE;

bool g_irqFired = false;
uint8_t g_currentDrive = 0;
uint32_t* fdc_buffer;

void i686_fdcInterruptHandler(Registers regs)
{
    g_irqFired = true;
}

void i686_fdcWaitIrq()
{
    while (!g_irqFired);
    g_irqFired = false;
}

void i686_fdcInitializeDma()
{
    i686_dmaMaskChannel(FDC_CHANNEL);
    i686_dmaResetFlipFlop(false);
    i686_dmaSetChannelAddr(FDC_CHANNEL, (uint32_t)fdc_buffer);
    i686_dmaResetFlipFlop(false);
    i686_dmaSetChannelCounter(FDC_CHANNEL, 0x23ff); // count to 0x23ff (number of bytes in a 3.5" floopy disk track)
    i686_dmaUnmaskChannel(FDC_CHANNEL);
}

void i686_fdcDmaRead()
{
    i686_dmaSetMode(FDC_CHANNEL, DMA_MODE_MASK_READ_TRANSFER | DMA_MODE_MASK_AUTO | DMA_MODE_MASK_TRANSFER_SINGLE);
}

void i686_fdcDmaWrite()
{
    i686_dmaSetMode(FDC_CHANNEL, DMA_MODE_MASK_WRITE_TRANSFER | DMA_MODE_MASK_AUTO | DMA_MODE_MASK_TRANSFER_SINGLE);
}

void i686_fdcWriteDor(uint8_t flags)
{
    i686_outb(FDC_PORT_DOR, flags);
}

uint8_t i686_fdcReadMsr()
{
    return i686_inb(FDC_PORT_MSR);
}

void i686_fdcDisableController()
{

	i686_fdcWriteDor(0);
}

void i686_fdcEnableController()
{
	i686_fdcWriteDor( FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
}

void i686_fdcSendCommand(uint8_t cmd)
{
    uint8_t msr;
    while(true)
    {
        msr = i686_fdcReadMsr();
        /*if(!(msr & FDC_MSR_MASK_BUSY) && (msr & FDC_MSR_MASK_DATAREG))*/
        if(msr & FDC_MSR_MASK_DATAREG)
        {
            i686_outb(FDC_PORT_FIFO, cmd);
            return;
        }

        printf("hello");
    }
}

uint8_t i686_fdcReadData()
{
    uint8_t msr;
    while(true)
    {
        msr = i686_fdcReadMsr();
        if(!(msr & FDC_MSR_MASK_BUSY) && (msr & FDC_MSR_MASK_DATAREG))
        {
            return i686_inb(FDC_PORT_FIFO);;
        }
    }
}

void i686_fdcConfigureDrive(uint32_t step_rate, uint32_t head_load_time, uint32_t head_unload_time, bool dma)
{
	uint32_t data = 0;

	//! send command
	i686_fdcSendCommand (FDC_CMD_SPECIFY);

	data = ( (step_rate & 0xf) << 4) | (head_unload_time & 0xf);
    i686_fdcSendCommand(data);

	data = (head_load_time) << 1 | (dma==false) ? 0 : 1;
    i686_fdcSendCommand (data);
}

void i686_fdcSelectDataRate(DATA_RATE rate)
{
    i686_outb(FDC_PORT_CCR, rate);
}

void i686_fdcCheckInterruptStatus(uint32_t* st0, uint32_t* cyl)
{
	i686_fdcSendCommand(FDC_CMD_CHECK_INT);

	*st0 = i686_fdcReadData();
	*cyl = i686_fdcReadData();
}

void i686_fdcControlMotor(bool is_on)
{

	// sanity check: invalid drive
	if (g_currentDrive > 3)
		return;

	uint32_t motor = 0;

	// select the correct mask based on current drive
	switch (g_currentDrive) {

		case 0:
			motor = FDC_DOR_MASK_DRIVE0_MOTOR;
			break;
		case 1:
			motor = FDC_DOR_MASK_DRIVE1_MOTOR;
			break;
		case 2:
			motor = FDC_DOR_MASK_DRIVE2_MOTOR;
			break;
		case 3:
			motor = FDC_DOR_MASK_DRIVE3_MOTOR;
			break;
	}

	// turn on or off the motor of that drive
	if (is_on)
        i686_fdcWriteDor(g_currentDrive | motor | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
	else
        i686_fdcWriteDor(FDC_DOR_MASK_RESET);

	// in all cases; wait a little bit for the motor to spin up/turn off
	//sleep (50);
}

bool i686_fdcCalibrate()
{
	uint32_t st0, cyl;

	// turn on the motor
	i686_fdcControlMotor(true);

	for (int i = 0; i < 10; i++)
    {
		// send command
		i686_fdcSendCommand(FDC_CMD_CALIBRATE);
		i686_fdcSendCommand(g_currentDrive);
		i686_fdcWaitIrq();
		i686_fdcCheckInterruptStatus (&st0, &cyl);

		// did we find cylinder 0? if so, we are done
		if (!cyl)
        {
            i686_fdcControlMotor(false);
			return true;
        }
	}

    i686_fdcControlMotor(false);
	return false;
}

void i686_fdcSetCurrentDrive(uint8_t drive)
{
    if(drive >= 4)
        return;

    i686_fdcWriteDor(drive | FDC_DOR_MASK_RESET);
    g_currentDrive = drive;
}

void i686_fdcResetController()
{
	uint32_t st0, cyl;

	// reset the controller
	i686_fdcDisableController();
	i686_fdcEnableController();
	i686_fdcWaitIrq();

	// send CHECK_INT/SENSE INTERRUPT command to all drives
	for (int i=0; i<4; i++)
        i686_fdcCheckInterruptStatus (&st0,&cyl);

	// transfer speed 500kb/s
	i686_fdcSelectDataRate(DATA_RATE_500KPS);

	// pass mechanical drive info. steprate=3ms, unload time=240ms, load time=16ms
	i686_fdcConfigureDrive(3, 16, 240, true);

	// calibrate the disk
	i686_fdcCalibrate();
}

void i686_fdcSectorRead(uint8_t head, uint8_t track, uint8_t sector)
{
	uint32_t st0, cyl;

	// set the DMA for read transfer
	i686_fdcDmaRead();

	// read in a sector
	i686_fdcSendCommand(FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	i686_fdcSendCommand(head << 2 | g_currentDrive);
	i686_fdcSendCommand(track);
	i686_fdcSendCommand(head);
	i686_fdcSendCommand(sector);
	i686_fdcSendCommand(FDC_SECTOR_SIZE_512);
	i686_fdcSendCommand((( sector + 1 ) >= FDC_SECTOR_PER_TRACK ) ? FDC_SECTOR_PER_TRACK : sector + 1);
	i686_fdcSendCommand(FDC_GAP3_LENGTH_3_5);
	i686_fdcSendCommand(0xff);

    printf("here !\n");
	// wait for irq
	i686_fdcWaitIrq();

	// read status info
	for (int j=0; j<7; j++)
        i686_fdcReadData();

	// let FDC know we handled interrupt
	i686_fdcCheckInterruptStatus(&st0,&cyl);
}

bool i686_fdcSeek(uint32_t cyl, uint32_t head)
{
	uint32_t st0, cyl0;

	for (int i = 0; i < 10; i++ ) {

		// send the command
		i686_fdcSendCommand(FDC_CMD_SEEK);
		i686_fdcSendCommand((head) << 2 | g_currentDrive);
		i686_fdcSendCommand(cyl);

		// wait for the results phase IRQ
		i686_fdcWaitIrq();
		i686_fdcCheckInterruptStatus(&st0, &cyl0);

		// found the cylinder?
		if (cyl0 == cyl)
			return true;
	}

	return false;
}

void fdc_lba2chs(uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut)
{
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % FDC_SECTOR_PER_TRACK + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / FDC_SECTOR_PER_TRACK) / FDC_HEAD;

    // head = (LBA / sectors per track) % heads
    *headOut = (lba / FDC_SECTOR_PER_TRACK) % FDC_HEAD;
}

void i686_fdcInitialize()
{
    printf("Initializing FDC...\n");
    fdc_buffer = (uint32_t*)i686_physMemoryAllocBlocks(FDC_BUFFER_BLOCKSIZE);

    i686_fdcEnableController();
    i686_disableInterrupts();

    i686_fdcSetCurrentDrive(0x0);

    // pass mechanical drive info. steprate=3ms, unload time=240ms, load time=16ms
    //i686_fdcConfigureDrive(3, 16, 240, true);
    i686_fdcSelectDataRate(DATA_RATE_500KPS);

    i686_fdcInitializeDma();
    i686_IRQ_registerNewHandler(6, i686_fdcWaitIrq);
    
    i686_enableInterrupts();
    printf("Done !\n");

    uint16_t cylinder, sector, head;
    fdc_lba2chs(1, &cylinder, &sector, &head);

    i686_fdcSectorRead(head, cylinder,sector);

    for(int i = 0; i < 512; i++)
    {
        printf("0x%x ", *(uint32_t*)((uint32_t)(fdc_buffer) + 1));
    }
}