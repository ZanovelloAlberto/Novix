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
#include <stddef.h>
#include <stdbool.h>
#include <hal/io.h>
#include <hal/pit.h>
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

volatile const char logo[] = 
"\
                 _   _            _      \n\
                | \\ | | _____   _(_)_  __\n\
                |  \\| |/ _ \\ \\ / / \\ \\/ /\n\
                | |\\  | (_) \\ V /| |>  < \n\
                |_| \\_|\\___/ \\_/ |_/_/\\_\\ \n\n\
";

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
                promptShift(end, 1);    // delete that character
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
void shellExecute()
{
    
    if(strcmp(prompt, "help") == 0)
        helpCommand(argc, args);
    else if(strcmp(prompt, "clear") == 0)
        clr();
    else if(strcmp(prompt, "exit") == 0)
        panic();
    else if(strcmp(prompt, "dumpsector") == 0)
        dumpsectorCommand(argc, args);
    else
        printf("%s: Unknown command", prompt);

    printf("\n");
}

void helpCommand(int argc, char** argv)
{
    printf("%s", logo);
    putc('\n');

    puts("Supported command:\n");
    puts(" - help: display this message\n");
    puts(" - clear: clear the screen\n");
    puts(" - exit: halt the system (forever)\n");
    puts(" - dumpsector: read a sector on disk\n");
}

void dumpsectorCommand(int argc, char** argv)
{
    uint8_t* phys_buffer;

    if(argc > 2 || argc < 2)
    {
        puts("Usage: dumpsector <sector number>");
        return;
    }

    phys_buffer = (uint8_t*)FDC_readSectors(0, 1);    // let's hope it's identity mapped. will need to map it if necessary
    for(int i = 0; i < 512; i++)
    {
        printf("0x%x ", phys_buffer[i]);
        sleep(10);
    }

}