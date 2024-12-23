#include <stdint.h>
#include "stdio.h"

void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    clr();
    puts("hello world\n\r");
}
