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
#include <hal/pit.h>
#include <hal/pic.h>
#include <hal/io.h>
#include <memmgr/physmem_manager.h>
#include <scheduler/multitask.h>
#include <debug.h>
#include <stddef.h>
#include <memory.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define TIMEOUT 1000
#define FDC_CHANNEL 2
#define FDC_BUFFER_BLOCKSIZE (64 / 4)
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
    DATA_RATE_500KPS    = 0x00,
    DATA_RATE_250KPS    = 0x02,
    DATA_RATE_300KPS    = 0x01,
    DATA_RATE_1MPS      = 0x03,
}DATA_RATE;

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

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

bool g_irqFired = false;
uint8_t g_currentDrive = 0;
uint32_t* fdc_buffer = NULL;
mutex_t* fdc_lock;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

void FDC_interruptHandler(Registers regs);
bool FDC_waitIrq();
void FDC_initializeDma(uint32_t phys_buffer, uint32_t count);
void FDC_dmaRead();
void FDC_dmaWrite();
void FDC_writeDor(uint8_t flags);
uint8_t FDC_readMsr();
bool FDC_sendCommand(uint8_t cmd);
uint8_t FDC_readData();
void FDC_selectDataRate(DATA_RATE rate);
void FDC_checkInterruptStatus(uint32_t* st0, uint32_t* cyl);
void FDC_configureDrive(uint32_t step_rate, uint32_t head_load_time, uint32_t head_unload_time, bool dma);
void FDC_controlMotor(bool is_on);
void FDC_sectorRead(uint8_t head, uint8_t track, uint8_t sector, uint32_t phys_buffer);

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================


void FDC_interruptHandler(Registers regs)
{
    g_irqFired = true;

    // send EOI
    PIC_sendEndOfInterrupt(6);
}

bool FDC_waitIrq()
{
    uint32_t timeout = TIMEOUT;
    while (!g_irqFired && timeout--);

    if (timeout == 0)
        return false; // timed out

    g_irqFired = false;
    return true;
}

void FDC_initializeDma(uint32_t phys_buffer, uint32_t count)
{
    DMA_maskChannel(FDC_CHANNEL);
    DMA_resetFlipFlop(false);

    DMA_setChannelAddr(FDC_CHANNEL, (uint32_t)phys_buffer);
    DMA_resetFlipFlop(false);

    DMA_setChannelCounter(FDC_CHANNEL, (count - 1)); // counting from 0
    DMA_unmaskChannel(FDC_CHANNEL);
}

void FDC_dmaRead()
{
    DMA_setMode(FDC_CHANNEL, DMA_MODE_MASK_READ_TRANSFER | DMA_MODE_MASK_AUTO | DMA_MODE_MASK_TRANSFER_SINGLE);
}

void FDC_dmaWrite()
{
    DMA_setMode(FDC_CHANNEL, DMA_MODE_MASK_WRITE_TRANSFER | DMA_MODE_MASK_AUTO | DMA_MODE_MASK_TRANSFER_SINGLE);
}

void FDC_writeDor(uint8_t flags)
{
    outb(FDC_PORT_DOR, flags);
}

uint8_t FDC_readMsr()
{
    return inb(FDC_PORT_MSR);
}

bool FDC_sendCommand(uint8_t cmd)
{
    uint8_t msr;
    uint32_t timeout = TIMEOUT;

    for(int i = 0; i < timeout; i++)
    {
        msr = FDC_readMsr();
        if(!(msr & FDC_MSR_MASK_DATAIO) && (msr & FDC_MSR_MASK_DATAREG))
        {
            outb(FDC_PORT_FIFO, cmd);
            return true;
        }
    }

    return false; // timed out
}

uint8_t FDC_readData()
{
    uint8_t msr;
    uint32_t timeout = TIMEOUT;

    for(int i = 0; i < timeout; i++)
    {
        msr = FDC_readMsr();
        if((msr & FDC_MSR_MASK_DATAIO) && (msr & FDC_MSR_MASK_DATAREG))
        {
            return inb(FDC_PORT_FIFO);
        }
    }

    return -1; // timed out
}

void FDC_selectDataRate(DATA_RATE rate)
{
    outb(FDC_PORT_CCR, rate);
}

void FDC_checkInterruptStatus(uint32_t* st0, uint32_t* cyl)
{
	FDC_sendCommand(FDC_CMD_CHECK_INT);

	*st0 = FDC_readData();
	*cyl = FDC_readData();
}

void FDC_configureDrive(uint32_t step_rate, uint32_t head_load_time, uint32_t head_unload_time, bool dma)
{
	uint32_t data = 0;

	// send command
	FDC_sendCommand (FDC_CMD_SPECIFY);

	data = ( (step_rate & 0xf) << 4) | (head_unload_time & 0xf);
    FDC_sendCommand(data);

	data = ((head_load_time) << 1) | ((dma == true) ? 0 : 1);
    FDC_sendCommand (data);
}

void FDC_controlMotor(bool is_on)
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
        FDC_writeDor(g_currentDrive | motor | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
	else
        FDC_writeDor(g_currentDrive | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);

	// in all cases; wait a little bit for the motor to spin up/turn off
    sleep(50);
}

void FDC_sectorRead(uint8_t head, uint8_t track, uint8_t sector, uint32_t phys_buffer)
{
	uint32_t st0 = 0, cyl = 0;

	// set the DMA for read transfer
    FDC_initializeDma(phys_buffer, 512);
	FDC_dmaRead();

	// read in a sector
    FDC_sendCommand(FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	FDC_sendCommand(head << 2 | g_currentDrive);
	FDC_sendCommand(track);
	FDC_sendCommand(head);
	FDC_sendCommand(sector);
	FDC_sendCommand(FDC_SECTOR_SIZE_512);
	FDC_sendCommand((( sector + 1 ) >= FDC_SECTOR_PER_TRACK ) ? FDC_SECTOR_PER_TRACK : (sector + 1));
	FDC_sendCommand(FDC_GAP3_LENGTH_3_5);
	FDC_sendCommand(0xff);

	// wait for irq
	FDC_waitIrq();

    uint8_t ret[7];
	// read status info
	for(int j=0; j<7; j++)
        ret[j] = FDC_readData();
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================


void FDC_disableController()
{
    acquire_mutex(fdc_lock);

	FDC_writeDor(0);

    release_mutex(fdc_lock);
}

void FDC_enableController()
{
    acquire_mutex(fdc_lock);

	FDC_writeDor(g_currentDrive | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);

    release_mutex(fdc_lock);
}

bool FDC_calibrate()
{
	uint32_t st0, cyl;

    acquire_mutex(fdc_lock);

	// turn on the motor
	FDC_controlMotor(true);

	for (int i = 0; i < 10; i++)
    {
		// send command
		FDC_sendCommand(FDC_CMD_CALIBRATE);
		FDC_sendCommand(g_currentDrive);
		FDC_waitIrq();
		FDC_checkInterruptStatus (&st0, &cyl);

		// did we find cylinder 0? if so, we are done
		if (!cyl)
        {
            FDC_controlMotor(false);
            release_mutex(fdc_lock);
			return true;
        }
	}

    FDC_controlMotor(false);
    release_mutex(fdc_lock);
	return false;
}

void FDC_setCurrentDrive(uint8_t drive)
{
    if(drive >= 4)
        return;

    acquire_mutex(fdc_lock);
    FDC_writeDor(drive | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
    g_currentDrive = drive;
    release_mutex(fdc_lock);
}

void FDC_resetController()
{
	uint32_t st0, cyl;

    acquire_mutex(fdc_lock);

	// reset the controller
	FDC_disableController();
	FDC_enableController();
	FDC_waitIrq();

	// send CHECK_INT/SENSE INTERRUPT command to all drives
	for (int i=0; i<4; i++)
        FDC_checkInterruptStatus (&st0,&cyl);

	// transfer speed 500kb/s
	FDC_selectDataRate(DATA_RATE_500KPS);

	// pass mechanical drive info. steprate=3ms, unload time=240ms, load time=16ms
	FDC_configureDrive(3, 16, 240, true);

	// calibrate the disk
	FDC_calibrate();

    release_mutex(fdc_lock);
}

bool FDC_seek(uint32_t cyl, uint32_t head)
{
	uint32_t st0, cyl0;

    acquire_mutex(fdc_lock);

	for (int i = 0; i < 10; i++ ) {

		// send the command
		FDC_sendCommand(FDC_CMD_SEEK);
		FDC_sendCommand((head) << 2 | g_currentDrive);
		FDC_sendCommand(cyl);

		// wait for the results phase IRQ
		FDC_waitIrq();
		FDC_checkInterruptStatus(&st0, &cyl0);

		// found the cylinder?
		if (cyl0 == cyl)
        {
            release_mutex(fdc_lock);
            return true;
        }
			
	}

    release_mutex(fdc_lock);
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

void FDC_readSectors(void* buffer, uint16_t lba, uint8_t sector_count)
{
    uint16_t cylinder, sector, head;

    if(sector_count > 128 || (lba + sector_count) > 2880)
        return;     // cannot read we only have 64k of buffer or out of range !

    acquire_mutex(fdc_lock);
    FDC_controlMotor(true);

    for (size_t i = 0; i < sector_count; i++)
    {
        lba = lba + i;
        
        fdc_lba2chs(lba, &cylinder, &sector, &head);
        FDC_seek(cylinder, head);
        FDC_sectorRead(head, cylinder,sector, ((uint32_t)fdc_buffer + 512 * i));
    }

    FDC_controlMotor(false);

    memcpy(buffer, fdc_buffer, sector_count*512);
    release_mutex(fdc_lock);
}

void FDC_initialize()
{
    log_info("kernel", "Initializing FDC...");

    fdc_lock = create_mutex();

    fdc_buffer = (uint32_t*)PHYSMEM_AllocBlocks(FDC_BUFFER_BLOCKSIZE); // Let’s hope it doesn’t go over 16MB.

    if(fdc_buffer == NULL)
    {
        log_err("kernel", "Initialization Failed\n");
        return;
    }

    disableInterrupts();
    IRQ_registerNewHandler(6, (IRQHandler)FDC_interruptHandler);
    enableInterrupts();

    FDC_setCurrentDrive(0x0);
    FDC_resetController();
}