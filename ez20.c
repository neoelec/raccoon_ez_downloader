// SPDX-License-Identifier: GPL-2.0
/*
 * Easy-Downloader V2.0 for ATMEL 89C51/52/55 (Raccoon's MOD)
 * Copyright(c) 2019 Joo, Young Jin, neoelec@gmail.com, June 8, 2019
 */

#include <at89x51.h>
#include <stdint.h>
#include <stdio.h>

#include "ez_common.h"

#define PORT_DATA		P0
#define PORT_ADDR_LOW		P1
#define PORT_ADDR_HIGH		P2

#define PIN_VPP_SEL		P3_3

#define PIN_PROG		P2_7
#define PIN_RDY			P2_6
#define PIN_P2_6		P3_4
#define PIN_P2_7		P3_5
#define PIN_P3_3		P3_2
#define PIN_P3_6		P3_6
#define PIN_P3_7		P3_7

static const char *title = "\n Easy-Downloader V2.0 for ATMEL 89C51/52/55 (Raccoon's MOD)";
static const char *prompt = "\n >";
static const char *ok = "\n ok";

static uint16_t chksum;
static uint16_t count;
static uint16_t bytes;
static uint16_t nonblank;

static uint8_t signature[3];

static void __ez20_print_title(void)
{
	ez_put_str(title);
}

static void __ez20_print_prompt(void)
{
	ez_put_str(prompt);
}

static void __ez20_print_ok(void)
{
	ez_put_str(ok);
}

static inline void __ez20_set_vpp_5v(void)
{
	PIN_VPP_SEL = 1;
}

static inline void __ez20_set_vpp_12v(void)
{
	PIN_VPP_SEL = 0;
}

static inline void __ez20_set_prog_low(void)
{
	PIN_PROG = 0;
}

static inline void __ez20_set_prog_high(void)
{
	PIN_PROG = 1;
}

static inline void __ez20_pulse_prog(void)
{
	__ez20_set_prog_low();
	__asm__ ("nop");
	__asm__ ("nop");
	__ez20_set_prog_high();
}

static inline void __ez20_wait_rdy(void)
{
	PIN_RDY = 1;
	while (!PIN_RDY);
}

static inline void __ez20_set_pgm_mode_pins(uint8_t p2_6, uint8_t p2_7,
		uint8_t p3_3, uint8_t p3_6, uint8_t p3_7)
{
	PIN_P2_6 = p2_6;
	PIN_P2_7 = p2_7;
	PIN_P3_3 = p3_3;
	PIN_P3_6 = p3_6;
	PIN_P3_7 = p3_7;
}

static inline void __ez20_set_addr_port(uint16_t addr)
{
	PORT_ADDR_LOW = (uint8_t)(addr & 0xFF);
	PORT_ADDR_HIGH = (uint8_t)(addr >> 0x8) | 0x80;
}

static inline uint8_t __ez20_read_data_port(void)
{
	PORT_DATA = 0xFF;

	return PORT_DATA;
}

static inline void __ez20_write_data_port(uint8_t data)
{
	PORT_DATA = data;
}

#define AT89C_SIGN_BASE			0x30
#define AT89C_SIGN_OFFSET		0x1

#define AT89S_SIGN_BASE			0x0
#define AT89S_SIGN_OFFSET		0x100

static void ____ez20_read_signature(uint16_t base, uint16_t offset)
{
	uint16_t addr;
	unsigned int i;

	__ez20_set_vpp_5v();
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(0, 0, 0, 0, 0);

	addr = base;
	for (i = 0; i < ARRAY_SIZE(signature); i++, addr += offset) {
		__ez20_set_addr_port(addr);
		ez_delay_us(10);
		signature[i] = __ez20_read_data_port();
	}
}

static void __ez20_read_signature(void)
{
	____ez20_read_signature(AT89C_SIGN_BASE,
			AT89C_SIGN_OFFSET);

	if (signature[AT89X_SIGN_CHIP_ID] == 0xFF)
		____ez20_read_signature(AT89S_SIGN_BASE,
				AT89S_SIGN_OFFSET);
}

static const struct at89x_sz_data ez20_sz_data[] = {
	AT89X_SZ_DATA(0x51, 4096),	/* 89C51*/
	AT89X_SZ_DATA(0x61, 4096),	/* 89LV51*/
	AT89X_SZ_DATA(0x52, 8192),	/* 89C52*/
	AT89X_SZ_DATA(0x62, 8192),	/* 89LV51*/
	AT89X_SZ_DATA(0x55, 20480),	/* 89C55*/
	AT89X_SZ_DATA(0x65, 20480),	/* 89LV55*/
	AT89X_SZ_DATA(0xFF, 0),
};

static void __ez20_identify_chip(void)
{
	uint16_t addr;
	uint8_t tmp;

	__ez20_read_signature();

	bytes = ez_sz_data_get_bytes(signature[AT89X_SIGN_CHIP_ID]);

	__ez20_set_vpp_5v();
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(0, 0, 0, 1, 1);

	nonblank = 0;
	chksum = 0;
	for(addr = 0; addr < bytes; addr++) {
		__ez20_set_addr_port(addr);
		ez_delay_us(10);
		tmp = __ez20_read_data_port();
		chksum += tmp;
		if (tmp != 0xFF)
			nonblank++;
	}
}

static int __ez20_cmd_c_chksum(void)
{
	ez_put_str("\n CHKSUM = ");
	ez_put_hex16(~chksum + 1);

	return 0;
}

static int __ez20_cmd_d_dump_eeprom(void)
{
	uint16_t addr;
	uint16_t max_addr;
	uint8_t chkihx;
	uint8_t tmp;

	__ez20_set_vpp_5v();
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(0, 0, 0, 1, 1);

	max_addr = count ? count : bytes;
	chkihx = 0;
	for(addr = 0; addr < max_addr; addr++) {
		if (0x00 == (addr & 0x0F)) {
			ez_put_str("\n:");
			chkihx = (max_addr - addr) > 0x10 ? 0x10 : max_addr - addr;
			ez_put_hex8(chkihx);
			ez_put_hex16(addr);
			chkihx += (uint8_t)(addr >> 0x8);
			chkihx += (uint8_t)(addr & 0xFF);
			ez_put_hex8(INTEL_HEX_RECORD_TYPE_DATA);
			chkihx += INTEL_HEX_RECORD_TYPE_DATA;
		}
		__ez20_set_addr_port(addr);
		ez_delay_us(10);
		tmp = __ez20_read_data_port();
		chkihx += tmp;
		ez_put_hex8(tmp);
		if (0x0F == (addr & 0x0F))
			ez_put_hex8((~chkihx) + 0x01);
		ez_wait_xon();
	}

	if (0x00 != (addr & 0x0F)) {
		ez_put_hex8((~chkihx) + 0x1);
	}
	ez_put_str("\n:00000001FF");	/* EOF */

	return 0;
}

static int __ez20_cmd_e_erase(void)
{
	__ez20_set_vpp_12v();
	ez_delay_ms(10);
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(1, 0, 1, 0, 0);
	ez_delay_us(10);

	if (signature[AT89X_SIGN_VPP] == 0x06) {
		/* AT89S series and AT89C55WD */
		__ez20_set_prog_low();
		__asm__ ("nop");
		__asm__ ("nop");
		__ez20_set_prog_high();
	} else {
		/* AT89C series */
		__ez20_set_prog_low();
		ez_delay_ms(10);
		__ez20_set_prog_high();
	}
	ez_delay_ms(100);

	__ez20_set_vpp_5v();
	ez_delay_ms(10);

	return 0;
}

static int __ez20_cmd_f_flash_eeprom(void)
{
	uint16_t addr;
	uint8_t size, type, chkihx, seq;
	uint8_t packet, tmp;
	uint8_t chk_result;

	__ez20_set_vpp_5v();
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(0, 1, 1, 1, 1);

	ez_delay_us(10);
	if (signature[AT89X_SIGN_VPP] != 0x05)
		__ez20_set_vpp_12v();
	ez_delay_ms(10);

	size = 0x0;
	addr = 0x0;
	type = INTEL_HEX_RECORD_TYPE_DATA;
	chkihx = 0x0;
	seq = 0x0;
	packet = 0x0;
	chk_result = 0x0;
	while (1) {
		putchar(XON);
		tmp = getchar();
		putchar(XOFF);

		if (tmp == ':') {
			if (chk_result)
				putchar(chk_result);
			size = 0x0;
			addr = 0x0;
			chkihx = 0x0;
			seq = 0x0;
			chk_result = 'x';
			continue;
		} else if (tmp >= '0' && tmp <= '9') {
			tmp = tmp - '0';
		} else if (tmp >= 'a' && tmp <= 'f') {
			tmp = tmp - 'a' + 0x0A;
		} else if (tmp >= 'A' && tmp <= 'F') {
			tmp = tmp - 'A' + 0x0A;
		} else {
			continue;
		}

		if (seq % 2)
			packet = (packet << 0x4) | tmp;
		else {
			packet = tmp;
			seq++;
			continue;
		}

		if (seq < 2)
			size = packet;
		else if (seq < 6)
			addr = (addr << 0x8) | packet;
		else if (seq < 8)
			type = packet;
		else if (size && type == INTEL_HEX_RECORD_TYPE_DATA) {
			__ez20_set_addr_port(addr++);
			ez_delay_us(10);
			__ez20_write_data_port(packet);
			__ez20_pulse_prog();
			__ez20_wait_rdy();
			size--;
		} else {
			chkihx = (~chkihx) + 0x1;
			if (packet == chkihx)
				chk_result = '.';

			if (type == INTEL_HEX_RECORD_TYPE_EOF)
				break;
		}

		seq++;
		chkihx += packet;
	}

	putchar(XON);

	__ez20_set_vpp_5v();
	ez_delay_ms(10);

	return 0;
}

static int __ez20_cmd_g_get_info(void)
{
	__ez20_identify_chip();

	if (signature[AT89X_SIGN_CHIP_ID] == 0xFF) {
		ez_put_str("\n Not found 89Cxx");
		return 0;
	}

	ez_put_str("\n found 89C");
	ez_put_hex8(signature[AT89X_SIGN_CHIP_ID]);
	if (signature[AT89X_SIGN_VPP] == 0x05)
		ez_put_str("-5V");
	else
		ez_put_str("-12V");

	ez_put_str("\n nonblank ");
	ez_put_dec(nonblank);
	ez_put_str(" bytes");
	ez_put_str("\n bytes counter ");
	ez_put_dec(count);

	return 0;
}

static int __ez20_cmd_i_signiture(void)
{
	__ez20_read_signature();

	ez_put_str("\n Manufacture : ");
	ez_put_hex8(signature[AT89X_SIGN_VENDOR]);
	ez_put_str("\n Chip ID     : ");
	ez_put_hex8(signature[AT89X_SIGN_CHIP_ID]);
	ez_put_str("\n VPP Code    : ");
	ez_put_hex8(signature[AT89X_SIGN_VPP]);

	return 0;
}

static int __ez20_cmd_l_lock(void)
{
	__ez20_set_vpp_12v();
	ez_delay_ms(10);
	__ez20_set_prog_high();

	__ez20_set_pgm_mode_pins(1, 1, 1, 1, 1);
	ez_delay_ms(10);
	__ez20_pulse_prog();
	ez_delay_ms(5);

	__ez20_set_pgm_mode_pins(1, 1, 1, 0, 0);
	ez_delay_ms(10);
	__ez20_pulse_prog();
	ez_delay_ms(5);

	__ez20_set_pgm_mode_pins(1, 0, 1, 1, 0);
	ez_delay_ms(10);
	__ez20_pulse_prog();
	ez_delay_ms(5);

	return 0;
}

static int __ez20_cmd_p_pgm_parameter(void)
{
	__ez20_identify_chip();

	ez_put_hex8(signature[AT89X_SIGN_CHIP_ID]);
	putchar(',');
	ez_put_dec(nonblank);
	putchar(',');
	ez_put_dec(count);

	return 0;
}

static int __ez20_cmd_q_read_lock_bit(void)
{
	__ez20_set_vpp_12v();
	ez_delay_ms(10);
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(1, 1, 0, 1, 0);
	ez_delay_ms(10);

	ez_put_bin8(__ez20_read_data_port());

	__ez20_set_vpp_5v();
	ez_delay_ms(10);

	return 0;
}

static int __ez20_cmd_r_read(void)
{
	uint16_t addr;
	uint8_t tmp;

	__ez20_set_vpp_5v();
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(0, 0, 0, 1, 1);

	chksum = 0;
	for(addr = 0; addr < count; addr++) {
		__ez20_set_addr_port(addr);
		ez_delay_us(10);
		tmp = __ez20_read_data_port();
		chksum += tmp;
		ez_put_hex8(tmp);
		ez_wait_xon();
	}

	return 0;
}

static int __ez20_cmd_s_set_counter(void)
{
	count = ez_get_dec();

	return 0;
}

static int __ez20_cmd_w_write(void)
{
	uint16_t addr;
	uint8_t tmp;

	__ez20_set_vpp_5v();
	__ez20_set_prog_high();
	__ez20_set_pgm_mode_pins(0, 1, 1, 1, 1);

	ez_delay_us(10);
	if (signature[AT89X_SIGN_VPP] != 0x05)
		__ez20_set_vpp_12v();
	ez_delay_ms(10);

	chksum = 0;
	for (addr = 0; addr < count; addr++) {
		putchar(XON);
		tmp = getchar();
		putchar(XOFF);
		__ez20_set_addr_port(addr);
		ez_delay_us(10);
		__ez20_write_data_port(tmp);
		chksum += tmp;
		__ez20_pulse_prog();
		__ez20_wait_rdy();
	}

	__ez20_set_vpp_5v();
	ez_delay_ms(10);

	return 0;
}

static int __ez20_cmd_cr_lf(void)
{
	__ez20_print_title();
	__ez20_print_prompt();

	return -EINVAL;
}

static const struct ez_cmd ez20_cmd_list[] = {
	ADD_CMD_HANDLER('c', __ez20_cmd_c_chksum, "show checksum"),
	ADD_CMD_HANDLER('d', __ez20_cmd_d_dump_eeprom, "dump eeprom (intel hex)"),
	ADD_CMD_HANDLER('e', __ez20_cmd_e_erase, "erase block"),
	ADD_CMD_HANDLER('f', __ez20_cmd_f_flash_eeprom, "flash eeprom (intel hex)"),
	ADD_CMD_HANDLER('g', __ez20_cmd_g_get_info, "get chip info"),
	ADD_CMD_HANDLER('i', __ez20_cmd_i_signiture, "show signature info"),
	ADD_CMD_HANDLER('l', __ez20_cmd_l_lock, "lock the chip"),
	ADD_CMD_HANDLER('p', __ez20_cmd_p_pgm_parameter, "show program parameter"),
	ADD_CMD_HANDLER('q', __ez20_cmd_q_read_lock_bit, "read lock bit"),
	ADD_CMD_HANDLER('r', __ez20_cmd_r_read, "read code"),
	ADD_CMD_HANDLER('s', __ez20_cmd_s_set_counter, "set number of byte"),
	ADD_CMD_HANDLER('w', __ez20_cmd_w_write, "write byte"),
	ADD_CMD_HANDLER('\n', __ez20_cmd_cr_lf, NULL),
	ADD_CMD_HANDLER('\r', __ez20_cmd_cr_lf, NULL),
	ADD_CMD_HANDLER('\0', NULL, NULL),
};

void main(void)
{
	uint8_t command;
	int err;

	ez_uart_init();

	ez_cmd_init(ez20_cmd_list);
	ez_sz_data_init(ez20_sz_data);

	__ez20_print_title();
	__ez20_print_prompt();

	while (1) {
		command = ez_get_command();
		err = ez_cmd_run(command);

		if (err == 0) {
			__ez20_print_ok();
			__ez20_print_prompt();
		}
	}
}
