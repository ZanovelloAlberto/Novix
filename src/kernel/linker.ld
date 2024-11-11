/*
 * File: linker.ld
 *
 * Description:
 *      linker file for the kernel code
 *
 * Author: Novice
 * last modification: 11/9th/2024
 */


OUTPUT_FORMAT(binary)
ENTRY(_start)
SECTIONS
{
    . = 0x20000;
    .text : {*(.text)}
    .data : {*(.data)}
    .bss : {*(.bss)}
}
