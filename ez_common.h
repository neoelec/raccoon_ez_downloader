#ifndef __EZ_COMMON_H__
#define __EZ_COMMON_H__

#include <stdint.h>

#define ARRAY_SIZE(__a)		(sizeof(__a) / sizeof(__a[0]))

#define XON			0x11
#define XOFF			0x13

/* TODO: 'errno.h' of sdcc does not have 'EINVAL' */
#define EINVAL			22	/* Invalid argument */

void ez_uart_init(void);

void ez_delay_us(uint8_t us);

void ez_delay_ms(uint16_t ms);

void ez_put_str(const uint8_t *string);
uint8_t ez_getchar_echo(void);

void ez_put_hex8(uint8_t n);
void ez_put_hex16(uint16_t n);
void ez_put_bin8(uint8_t n);
void ez_put_dec(unsigned int n);
unsigned int ez_get_dec(void);

uint8_t ez_get_command(void);
uint8_t ez_get_char(void);

void ez_wait_xon(void);

struct ez_cmd {
	uint8_t command;
	int (*handler)(void);
	const char *help;
};

#define ADD_CMD_HANDLER(__command, __handler, __help)			\
	{ .command = __command, .handler = __handler, .help = __help, }

void ez_cmd_init(const struct ez_cmd *list);
int ez_cmd_run(uint8_t command);

struct at89x_sz_data {
	uint8_t chip_id;
	unsigned int bytes;
};

#define AT89X_SZ_DATA(__chip_id, __bytes)				\
	{ .chip_id = __chip_id, .bytes = __bytes, }

void ez_sz_data_init(const struct at89x_sz_data *data);
unsigned int ez_sz_data_get_bytes(uint8_t chip_id);

#define AT89X_SIGN_VENDOR		0
#define AT89X_SIGN_CHIP_ID		1
#define AT89X_SIGN_VPP			2

#define INTEL_HEX_RECORD_TYPE_DATA	0x0
#define INTEL_HEX_RECORD_TYPE_EOF	0x1

#endif /* __EZ_COMMON_H__ */
