// SPDX-License-Identifier: GPL-2.0
/*
 * Easy-Downloader V1.3 for ATMEL 89C2051/4051 (Raccoon's MOD)
 * Copyright(c) 2019 Joo, Young Jin, neoelec@gmail.com, June 8, 2019
 */

#include <at89x51.h>
#include <stdint.h>
#include <stdio.h>

#include "ez_common.h"

#define PORT_DATA		P1

#define PIN_LATCH_EN		P3_7

#define PIN_VPP_EN		P1_4
#define PIN_VPP_SEL		P3_5

#define PIN_PROG		P3_2
#define PIN_RDY			P3_3
#define PIN_XTAL		P3_4
#define PIN_P3_3		P1_2
#define PIN_P3_4		P1_1
#define PIN_P3_5		P1_0

static const char *title = "\n Easy-Downloader V1.3 for ATMEL 89C2051/4051 (Raccoon's MOD)";
static const char *prompt = "\n >";
static const char *ok = "\n ok";

static uint16_t chksum;
static uint16_t count;
static uint16_t bytes;
static uint16_t nonblank;

static uint8_t signature[3];

static void __ez13_print_title(void)
{
	ez_put_str(title);
}

static void __ez13_print_prompt(void)
{
	ez_put_str(prompt);
}

static void __ez13_print_ok(void)
{
	ez_put_str(ok);
}

static void __ez13_update_latched_pin(void)
{
	ez_delay_us(20);
	PIN_LATCH_EN = 1;
	ez_delay_us(20);
	PIN_LATCH_EN = 0;
	ez_delay_us(20);
}

static inline void __ez13_turn_off_vpp(void)
{
	PIN_VPP_EN = 1;

	__ez13_update_latched_pin();
}

static inline void __ez13_turn_on_vpp(void)
{
	PIN_VPP_EN = 0;

	__ez13_update_latched_pin();
}

static inline void __ez13_set_vpp_5v(void)
{
	PIN_VPP_SEL = 1;
}

static inline void __ez13_set_vpp_12v(void)
{
	PIN_VPP_SEL = 0;
}

static inline void __ez13_set_prog_low(void)
{
	PIN_PROG = 0;
}

static inline void __ez13_set_prog_high(void)
{
	PIN_PROG = 1;
}

static inline void __ez13_pulse_prog(void)
{
	__ez13_set_prog_low();
	__asm__ ("nop");
	__asm__ ("nop");
	__ez13_set_prog_high();
}

static inline void __ez13_increase_addr(void)
{
	ez_delay_us(20);
	PIN_XTAL = 1;
	ez_delay_us(20);
	PIN_XTAL = 0;
}

static void __ez13_reset_power(void)
{
	PIN_XTAL = 0;
	PIN_RDY = 1;

	__ez13_set_prog_low();
	__ez13_set_vpp_5v();
	__ez13_turn_off_vpp();
	ez_delay_ms(20);

	__ez13_set_prog_high();
	__ez13_set_vpp_5v();
	__ez13_turn_on_vpp();
	ez_delay_ms(20);
}

static inline void __ez13_wait_rdy(void)
{
	PIN_RDY = 1;
	while (!PIN_RDY);
}

#define WRITE_CODE_DATA		0, 1, 1
#define READ_CODE_DATA		0, 0, 1
#define WRITE_LOCK_BIT_1	1, 1, 1
#define WRITE_LOCK_BIT_2	1, 1, 0
#define CHIP_ERASE		1, 0, 0
#define READ_SIGNATURE_BYTE	0, 0, 0

static inline void __ez13_set_pgm_mode_pins(uint8_t p3_3, uint8_t p3_4,
		uint8_t p3_5)
{
	PIN_P3_3 = p3_3;
	PIN_P3_4 = p3_4;
	PIN_P3_5 = p3_5;

	__ez13_update_latched_pin();
}

static inline uint8_t __ez13_read_data_port(void)
{
	PORT_DATA = 0xFF;
	ez_delay_us(20);

	return PORT_DATA;
}

static inline void __ez13_write_data_port(uint8_t data)
{
	PORT_DATA = data;
}

static void __ez13_read_signature(void)
{
	unsigned int i;

	__ez13_reset_power();

	__ez13_set_prog_high();
	__ez13_set_pgm_mode_pins(READ_SIGNATURE_BYTE);

	for (i = 0; i < ARRAY_SIZE(signature); i++) {
		signature[i] = __ez13_read_data_port();
		__ez13_increase_addr();
	}
}

static const struct at89x_sz_data ez13_sz_data[] = {
	AT89X_SZ_DATA(0x21, 2048),	/* 89C2051 */
	AT89X_SZ_DATA(0x41, 4096),	/* 89C4051 */
	AT89X_SZ_DATA(0x23, 2048),	/* 89S2051 */
	AT89X_SZ_DATA(0x43, 4096),	/* 89S4051 */
	AT89X_SZ_DATA(0xFF, 0),
};

static int __ez13_identify_chip(void)
{
	unsigned int i;
	uint8_t tmp;

	__ez13_read_signature();

	bytes = ez_sz_data_get_bytes(signature[AT89X_SIGN_CHIP_ID]);

	__ez13_reset_power();

	__ez13_set_prog_low();
	__ez13_set_pgm_mode_pins(READ_CODE_DATA);

	nonblank = 0;
	chksum = 0;
	for(i = 0; i < bytes; i++) {
		__ez13_set_prog_high();
		tmp = __ez13_read_data_port();
		chksum += tmp;
		if (tmp != 0xFF)
			nonblank++;
		__ez13_increase_addr();
	}

	return 0;
}

static int __ez13_cmd_c_chksum(void)
{
	ez_put_str("\n CHKSUM = ");
	ez_put_hex16(~chksum + 1);

	return 0;
}

static int __ez13_cmd_d_dump_eeprom(void)
{
	uint16_t addr;
	uint16_t max_addr;
	uint8_t chkihx;
	uint8_t tmp;

	__ez13_reset_power();

	__ez13_set_prog_low();
	__ez13_set_pgm_mode_pins(READ_CODE_DATA);

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
		__ez13_set_prog_high();
		tmp = __ez13_read_data_port();
		chkihx += tmp;
		ez_put_hex8(tmp);
		if (0x0F == (addr & 0x0F))
			ez_put_hex8((~chkihx) + 0x01);
		__ez13_increase_addr();
		ez_wait_xon();
	}

	if (0x00 != (addr & 0x0F)) {
		ez_put_hex8((~chkihx) + 0x1);
	}
	ez_put_str("\n:00000001FF");	/* EOF */

	return 0;
}

static int __ez13_cmd_e_erase(void)
{
	unsigned int i;

	__ez13_set_prog_high();
	__ez13_set_pgm_mode_pins(CHIP_ERASE);

	__ez13_set_vpp_12v();
	__ez13_turn_on_vpp();
	ez_delay_ms(20);

	for (i = 0; i < 10; i++) {
		__ez13_set_prog_low();
		ez_delay_ms(10);
		__ez13_set_prog_high();
		ez_delay_ms(1);
	}
	__ez13_reset_power();

	return 0;
}

static void __ez13_set_addr(uint16_t addr_curr, uint16_t addr_new)
{
	uint16_t i;

	if (addr_curr == addr_new)
		return;

	for (i = addr_curr; i < addr_new; i++) {
		__ez13_write_data_port(0xFF);
		__ez13_pulse_prog();
		__ez13_wait_rdy();
	}
}

static int __ez13_cmd_f_flash_eeprom(void)
{
	uint16_t addr, addr_curr;
	uint8_t size, type, chkihx, seq;
	uint8_t packet, tmp;
	uint8_t chk_result;

	__ez13_set_prog_high();
	__ez13_set_pgm_mode_pins(WRITE_CODE_DATA);

	__ez13_reset_power();
	__ez13_set_vpp_12v();
	__ez13_turn_on_vpp();
	ez_delay_ms(20);

	size = 0x0;
	addr = 0x0;
	addr_curr = 0x0;
	type = INTEL_HEX_RECORD_TYPE_DATA;
	chkihx = 0x0;
	seq = 0xF;
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
			__ez13_set_addr(addr_curr, addr);
			addr++;
			addr_curr = addr;
			__ez13_write_data_port(packet);
			__ez13_pulse_prog();
			__ez13_wait_rdy();
			__ez13_increase_addr();
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

	__ez13_set_vpp_5v();
	__ez13_turn_off_vpp();

	return 0;
}

static int __ez13_cmd_g_get_info(void)
{
	__ez13_identify_chip();

	if (signature[AT89X_SIGN_CHIP_ID] != 0x21 &&
	    signature[AT89X_SIGN_CHIP_ID] != 0x41) {
		ez_put_str("\n No found 89Cxx");
		return 0;
	}

	ez_put_str("\n found 89C");
	if (signature[AT89X_SIGN_CHIP_ID] == 0x21)
		ez_put_str("2051-12V");
	else
		ez_put_str("4051-12V");

	ez_put_str("\n nonblank ");
	ez_put_dec(nonblank);
	ez_put_str(" bytes");
	ez_put_str("\n bytes counter ");
	ez_put_dec(count);

	return 0;
}

static int __ez13_cmd_i_signiture(void)
{
	__ez13_read_signature();

	ez_put_str("\n Manufacture : ");
	ez_put_hex8(signature[AT89X_SIGN_VENDOR]);
	ez_put_str("\n Chip ID     : ");
	ez_put_hex8(signature[AT89X_SIGN_CHIP_ID]);

	return 0;
}

static int __ez13_cmd_l_lock(void)
{
	__ez13_set_pgm_mode_pins(WRITE_LOCK_BIT_1);
	__ez13_set_vpp_12v();
	ez_delay_ms(20);
	__ez13_pulse_prog();

	__ez13_set_pgm_mode_pins(WRITE_LOCK_BIT_2);
	__ez13_set_vpp_12v();
	ez_delay_ms(20);
	__ez13_pulse_prog();

	__ez13_set_vpp_5v();
	__ez13_turn_off_vpp();

	return 0;
}

static int __ez13_cmd_p_pgm_parameter(void)
{
	__ez13_identify_chip();

	ez_put_hex8(signature[AT89X_SIGN_CHIP_ID]);
	putchar(',');
	ez_put_dec(nonblank);
	putchar(',');
	ez_put_dec(count);

	return 0;
}

static int __ez13_cmd_r_read(void)
{
	unsigned int i;
	uint8_t tmp;

	__ez13_reset_power();

	__ez13_set_prog_low();
	__ez13_set_pgm_mode_pins(READ_CODE_DATA);

	chksum = 0;
	for(i = 0; i < count; i++) {
		__ez13_set_prog_high();
		tmp = __ez13_read_data_port();
		ez_put_hex8(tmp);
		chksum += tmp;
		__ez13_increase_addr();
		ez_wait_xon();
	}

	return 0;
}

static int __ez13_cmd_s_set_counter(void)
{
	count = ez_get_dec();

	return 0;
}

static int __ez13_cmd_w_write(void)
{
	unsigned int i;
	uint8_t tmp;

	__ez13_set_prog_high();
	__ez13_set_pgm_mode_pins(WRITE_CODE_DATA);

	__ez13_reset_power();
	__ez13_set_vpp_12v();
	__ez13_turn_on_vpp();
	ez_delay_ms(20);

	chksum = 0;
	for (i = 0; i < count; i++) {
		putchar(XON);
		tmp = getchar();
		putchar(XOFF);
		__ez13_write_data_port(tmp);
		chksum += tmp;
		__ez13_pulse_prog();
		__ez13_wait_rdy();
		__ez13_increase_addr();
	}

	__ez13_set_vpp_5v();
	__ez13_turn_off_vpp();

	return 0;
}

static int __ez13_cmd_cr_lf(void)
{
	__ez13_print_title();
	__ez13_print_prompt();

	return -EINVAL;
}

static const struct ez_cmd ez13_cmd_list[] = {
	ADD_CMD_HANDLER('c', __ez13_cmd_c_chksum, "show checksum"),
	ADD_CMD_HANDLER('d', __ez13_cmd_d_dump_eeprom, "dump eeprom (intel hex)"),
	ADD_CMD_HANDLER('e', __ez13_cmd_e_erase, "erase block"),
	ADD_CMD_HANDLER('f', __ez13_cmd_f_flash_eeprom, "flash eeprom (intel hex)"),
	ADD_CMD_HANDLER('g', __ez13_cmd_g_get_info, "get chip info"),
	ADD_CMD_HANDLER('i', __ez13_cmd_i_signiture, "show signature info"),
	ADD_CMD_HANDLER('l', __ez13_cmd_l_lock, "lock the chip"),
	ADD_CMD_HANDLER('p', __ez13_cmd_p_pgm_parameter, "show program parameter"),
	ADD_CMD_HANDLER('r', __ez13_cmd_r_read, "read code"),
	ADD_CMD_HANDLER('s', __ez13_cmd_s_set_counter, "set number of byte"),
	ADD_CMD_HANDLER('w', __ez13_cmd_w_write, "write byte"),
	ADD_CMD_HANDLER('\n', __ez13_cmd_cr_lf, NULL),
	ADD_CMD_HANDLER('\r', __ez13_cmd_cr_lf, NULL),
	ADD_CMD_HANDLER('\0', NULL, NULL),
};

void main(void)
{
	uint8_t command;
	int err;

	__ez13_reset_power();

	ez_uart_init();

	ez_cmd_init(ez13_cmd_list);
	ez_sz_data_init(ez13_sz_data);

	__ez13_print_title();
	__ez13_print_prompt();

	while (1) {
		command = ez_get_command();
		err = ez_cmd_run(command);

		if (err == 0) {
			__ez13_print_ok();
			__ez13_print_prompt();
		}
	}
}
