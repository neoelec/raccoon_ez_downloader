// SPDX-License-Identifier: GPL-2.0
/*
 * Easy-Downloader V2.0 for ATMEL 89C51/52/55 (Raccoon's MOD)
 * Copyright(c) 2019 Joo, Young Jin, neoelec@gmail.com, June 8, 2019
 */

#include <8051.h>
#include <stdint.h>
#include <stdio.h>

#include "ez_common.h"

#define ARRAY_SIZE(__a)		(sizeof(__a) / sizeof(__a[0]))

#define XON			0x11
#define XOFF			0x13

void ez_uart_init(void)
{
	uint8_t dummy;

	SCON = 0x52;	/* 8-bit UART mode */
	TMOD = 0x20;	/* timer 1 mode 2 auto reload */
	TH1= 0xFD;	/* 9600 8n1 */
	TR1 = 1;	/* run timer1 */

	dummy = SBUF;
}

static void __putchar(int ch)
{
	while(!TI);
	TI = 0;
	SBUF = (uint8_t)ch;
}

int putchar(int ch)
{
	__putchar(ch);
	if (ch == '\n')
		__putchar('\r');

	return ch;
}

int getchar(void)
{
	while(!RI);
	RI = 0;
	return SBUF;
}

void ez_delay_us(uint8_t us)
{
	uint8_t i;

	us = us >> 4;
	for (i = 1; i <= us; i++) ;
}

void ez_delay_ms(uint16_t ms)
{
	uint16_t i, j;

	for (i = 1; i <= ms; i++)
		for (j = 1; j <= 115; j++) ;
}

void ez_put_str(const uint8_t *string)
{
	while (*string != '\0')
		putchar(*string++);
}

uint8_t ez_getchar_echo(void)
{
	uint8_t character = getchar();

	putchar(character);

	return character;
}

static void __ez_put_hex(uint8_t n)
{
	if (n <= 9)
		putchar(n + '0');
	else
		putchar(n - 10 + 'A');
}

void ez_put_hex8(uint8_t n)
{
	__ez_put_hex(n >> 0x4);
	__ez_put_hex(n & 0x0F);
}

void ez_put_hex16(uint16_t n)
{
	ez_put_hex8((uint8_t)(n >> 0x8));
	ez_put_hex8((uint8_t)(n & 0xFF));
}

void ez_put_bin8(uint8_t n)
{
	uint8_t mask;

	for (mask = 0x80; mask != 0; mask = mask >> 0x1)
		putchar('0' + !!(n & mask));
}

void ez_put_dec(unsigned int n)
{
	char s[8];
	int i;

	s[ARRAY_SIZE(s) - 1] = '\0';

	for (i = ARRAY_SIZE(s) - 2; i >= 0; i--) {
		s[i] = (n % 10) + '0';
		n = n / 10;
		if (!n)
			break;
	}

	ez_put_str(&s[i]);
}

unsigned int ez_get_dec(void)
{
	uint8_t ch;
	unsigned int dec = 0;

	while (1) {
		putchar(XON);
		ch = ez_getchar_echo();
		if (ch == '\n' || ch == '\r')
			break;
		dec = dec * 10 + (unsigned int)(ch - '0');
	}

	 return dec;
}

uint8_t ez_get_command(void)
{
	if (RI)
		return ez_getchar_echo();

	return 0xFF;
}

uint8_t ez_get_char(void)
{
	if (RI)
		return getchar();

	return 0xFF;
}

void ez_wait_xon(void)
{
	if (XOFF == ez_get_char())
		while (XON != ez_get_char());
}

static const struct ez_cmd *cmd_list;

void ez_cmd_init(const struct ez_cmd *list)
{
	cmd_list = list;
}

static void __ez_cmd_help(void)
{
	unsigned int i;

	for (i = 0; cmd_list[i].handler != NULL; i++) {
		if (cmd_list[i].help != NULL) {
			ez_put_str("\n ");
			putchar(cmd_list[i].command);
			putchar(' ');
			ez_put_str(cmd_list[i].help);
		}
	}
}

int ez_cmd_run(uint8_t command)
{
	unsigned int i;

	if (command == '?') {
		__ez_cmd_help();
		return 0;
	}

	for (i = 0; cmd_list[i].handler != NULL; i++)
		if (cmd_list[i].command == command)
			return cmd_list[i].handler();

	return -EINVAL;
}

static const struct at89x_sz_data *sz_data;

void ez_sz_data_init(const struct at89x_sz_data *data)
{
	sz_data = data;
}

unsigned int ez_sz_data_get_bytes(uint8_t chip_id)
{
	unsigned int i;

	for (i = 0; sz_data[i].bytes != 0; i++)
		if (sz_data[i].chip_id == chip_id)
			return sz_data[i].bytes;

	return 0;
}
