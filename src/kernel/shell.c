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

#include <stdio.h>
#include <debug.h>
#include <drivers/vga_text.h>
#include <stddef.h>
#include <stdbool.h>
#include <hal/io.h>
#include <hal/pit.h>
#include <hal/physmem_manager.h>
#include <hal/virtmem_manager.h>
#include <hal/heap.h>
#include <hal/usermode.h>
#include <vfs/vfs.h>
#include <drivers/fdc.h>
#include <memory.h>
#include <string.h>
#include <shell.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define MAX_CHAR_PROMPT 256
#define MAX_CMD_ARGS    64

typedef enum {
    QUOTE_STATE_FREE,
    WAIT_FOR_SINGLE_QUOTE,
    WAIT_FOR_DOUBLE_QUOTE,
}QUOTE_FLAG_STATE;

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

char prompt[MAX_CHAR_PROMPT];
char* args[MAX_CMD_ARGS];
int argc;

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

void promptPurify(char* source)
{
    /*
    * strip leading and trailing spaces
    */

    short spaceCount = 0;
    char *index = source;

    // counting leading white spaces
    while(*index == ' ' || *index == '\t')
    {
        spaceCount++;
        index++;
    }

    index = source;
    source = source + spaceCount;

    //shifting the string to fill the spaces
    while(*source != '\0')
    {
        *index = *source;
        index++;
        source++;
    }

    *index = '\0';
    index--;

    //filling ending spaces with zero
    while(*index == ' ' || *index == '\t')
    {
        *index = '\0';
        index--;
    }
}

void promptShift(char* source, int places)
{
    char* index = source + places;

    if(*index == '\0') // if you shift the whole string
    {
        *source = '\0';
        return;
    }

    //shifting the string
    while(*index != '\0' && *source != '\0')
    {
        *source = *index;
        index++;
        source++;
    }

    *source = '\0';
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void shellRead()
{
    int position = 0;
    char c;

    memset(prompt, 0, MAX_CHAR_PROMPT);

    while(position < (MAX_CHAR_PROMPT - 1)) // save one character for '\0'
    {
        c = getchar();

        if(c == '\b')
        {
            if(position == 0)   // if empty buffer
                continue;   // nothing to do
            
            putc(c);
            position--;
            continue;
        }
        else if(c == '\n')
        {
            putc(c);
            prompt[position] = '\0';
            return;   
        }
        else
        {
            prompt[position] = c;
            putc(c);
        }

        position++;
    }

    putc('\n');
}

void shellParse()
{
    char *begin, *end;

    bool quoteFlag = false; // used to handle the "" or '' on the prompt
    QUOTE_FLAG_STATE quoteFlagState = QUOTE_STATE_FREE;

    memset(args, (int)NULL, MAX_CMD_ARGS);
    argc = 0;
    promptPurify(prompt);

    begin = prompt;
    end = begin;

    do
    {
        /*
        * when a quotation mark is uncountered for the first time the quoteFlag is set
        * and unset when uncountered for the second time
        */

        if(quoteFlagState == QUOTE_STATE_FREE)
        {
            switch (*end)
            {
            case '"':
                quoteFlag = true;
                quoteFlagState = WAIT_FOR_DOUBLE_QUOTE;
                promptShift(end, 1);    // get rid of that character
                break;

            case '\'':
                quoteFlag = true;
                quoteFlagState = WAIT_FOR_SINGLE_QUOTE;
                promptShift(end, 1);
                break;
            
            default:
                break;
            }
        }
        else if(quoteFlagState == WAIT_FOR_DOUBLE_QUOTE)
        {
            if(*end == '"')
            {
                quoteFlag = false;
                quoteFlagState = QUOTE_STATE_FREE;
                promptShift(end, 1);
            }
        }
        else
        {
            if(*end == '\'')
            {
                quoteFlag = false;
                quoteFlagState = QUOTE_STATE_FREE;
                promptShift(end, 1);
            }
        }


        // if the quoteFlag is set we don't need to split the spaces between the quotation mark
        if(!quoteFlag && (*end == ' ' || *end == '\t'))
        {

            *end = '\0';

            //add string to args
            args[argc] = begin;
            argc++;

            // set begin to the next string
            begin = end + 1;
        }

        end++;
    }while(*end != '\0' && argc < (MAX_CMD_ARGS - 1));  // save one character args must end with NULL

    args[argc] = begin; // adding the last remaining string in the args list
    argc++;
}

void helpCommand(int argc, char** argv);
void dumpsectorCommand(int argc, char** argv);
void physmeminfoCommand(int argc, char** argv);
void readfileCommand(int argc, char** argv);
void usermodecommand(int argc, char** argv);
void shellExecute()
{
    
    if(strcmp(prompt, "help") == 0)
        helpCommand(argc, args);
    else if(strcmp(prompt, "clear") == 0)
        VGA_clr();
    else if(strcmp(prompt, "exit") == 0)
        panic();
    else if(strcmp(prompt, "dumpsector") == 0)
        dumpsectorCommand(argc, args);
    else if(strcmp(prompt, "usermode") == 0)
        usermodecommand(argc, args);
    else if(strcmp(prompt, "physmeminfo") == 0)
        physmeminfoCommand(argc, args);
    else if(strcmp(prompt, "readfile") == 0)
        readfileCommand(argc, args);
    else
        printf("%s: Unknown command", prompt);

    puts("\n");
}

void helpCommand(int argc, char** argv)
{
    putc('\n');

    puts("  command");
    VGA_moveCursorTo(VGA_getCurrentLine(), 27);
    puts("description\n");
    puts("------------------------------------------------------------------------------\n");

    VGA_coloredPuts(" - help", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": display this message\n");

    VGA_coloredPuts(" - clear", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": clear the screen\n");

    VGA_coloredPuts(" - exit", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": halt the system (forever)\n");

    VGA_coloredPuts(" - dumpsector", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": read a sector on disk and display the content\n");

    VGA_coloredPuts(" - physmeminfo", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": physical memory manager information\n");

    VGA_coloredPuts(" - readfile", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": read a file from disk !\n");

    VGA_coloredPuts(" - usermode", VGA_COLOR_LIGHT_CYAN);
    VGA_moveCursorTo(VGA_getCurrentLine(), 25);
    puts(": jump and run a usermode program !\n");
}

void physmeminfoCommand(int argc, char** argv)
{
    physmem_info_t info;

    PHYSMEM_getMemoryInfo(&info);

    printf("bitmap size: %d\n", info.bitmapSize);
    printf("total block number: %d\n", info.totalBlockNumber);
    printf("total free block: %d\n", info.totalFreeBlock);
    printf("total used block: %d\n", info.totalUsedBlock);
}

void usermodecommand(int argc, char** argv)
{
    int fd1 = VFS_open("/userprog.bin", VFS_O_RDWR);

    if(fd1 < 0)
    {
        VGA_puts("failed to open the file\n");
        return;
    }

    VIRTMEM_mapPage ((void*)0x80000000, false);

    VFS_read(fd1, (void*)0x80000000, 4095);
    VFS_close(fd1);

    // void TSS_setKernelStack(uint32_t esp0); // for syscall !!
    switch_to_usermode(0x80000000+4095, 0x80000000);
}

void readfileCommand(int argc, char** argv)
{
    if(argc > 2 || argc < 2)
    {
        puts("Usage: readfile <path>");
        return;
    }

    uint8_t buffer[10];

    printf("opening %s\n", argv[1]);
    int fd1 = VFS_open(argv[1], VFS_O_RDWR);

    if(fd1 < 0)
    {
        VGA_puts("failed to open the file\n");
        return;
    }

    size_t read = 0;

    printf("content:\n");
    do
    {
        read = VFS_read(fd1, buffer, 9);
        buffer[read] = '\0';
        printf("%s", buffer);
    }while(read != 0);

    VFS_close(fd1);
}

void dumpsectorCommand(int argc, char** argv)
{
    uint8_t phys_buffer[512];

    if(argc > 2 || argc < 2)
    {
        puts("Usage: dumpsector <sector number>");
        return;
    }

    FDC_readSectors(phys_buffer, (uint16_t)strtol(argv[1], NULL, 0), 1);

    int c = 256;
    for(int i = 0; i < 2; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            printf("0x%x ", phys_buffer[j + (c * i)]);
            sleep(5);
        }
        puts("\n\rPress any key to continue\n\r");
        waitForKeyPress();
    }

}