#include "stdio.h"

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    clr();
    printf("Hello world from kernel!!!\n");

end:
    for (;;);
}