/*
 * File: util.h
 *
 * Description: header file for util.c
 *
 * Author: Novice
 * last modification: 11/9th/2024
 */

#pragma once

/** outb:
* Sends the given data (byte size) to the given I/O port. Defined in util.s
*
* @param port The I/O port to send the data to
* @param data The data to send to the I/O port
*/
void outb(unsigned short port, unsigned char data);