/*************************************************************************
 *  copy from https://github.com/Qunero/dnw4linux.git
 *
 *  Author: slash.huang <slash.linux.c@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define LED_MODE_IDX 3
#define LED_3_IDX    2
#define LED_2_IDX    1
#define LED_1_IDX    0
#define CMD_BUF_SZ   10
const char *dev = "/dev/usb_led0";

void init_data(unsigned char *data, unsigned char c)
{
	unsigned int i;

	for (i = 0; i < c; i++)
		data[i] = i;
}

/***************************
 * D0 : LED1
 * D1 : LED2
 * D2 : LED3
 * D3 : LED MODE
 *        0 : bit mode
 *        1 : cnt mode
 *
 * ************************/
int main(int argc, char *argv[])
{
	unsigned char *file_buffer = NULL;
	unsigned char data_buf[CMD_BUF_SZ] = {0};
	int fd_dev, led_id, led_on, led_mode;
	int opts, option_index = 0;

	/* getopt_long stores the option index here. */
	struct option long_options[] = {
		{"mode", required_argument, 0, 'm'},
		{"id", required_argument, 0, 'i'},
		{"on", required_argument, 0, 'o'},
		{"?",  no_argument, 0, '?'},
		{0, 0, 0, 0}
	};

	while (1) {
		opts = getopt_long (argc, argv,
		"m:i:o:", long_options, &option_index);

		/* Detect the end of the options. */
		if (opts == -1)
			break;

		switch (opts) {
			case 'm':
				led_mode = atoi(optarg);
				break;
			case 'i':
				led_id = atoi(optarg);
				break;

			case '?':
				printf(
				"./usb_led_cmd -i LED_NUM -o ON_OFF\r\n"
				"LED_NUM : 0 - 3\r\n"
				"ON/OFF :\r\n"
				"   ON : 1\r\n"
				"   OFF: 0\r\n"
				"./usb_led_cmd -m 0 -i 1 \r\n : led ID0 on\r\n"
				"./usb_led_cmd -m 0 -i 0 \r\n : led ID0 off\r\n"
				"./usb_led_cmd -m 0 -i 3 \r\n : led ID0 ID1 on\r\n"
				"./usb_led_cmd -m 0 -i 7 \r\n : led ID0 ID1 ID2 on\r\n");
				break;
			default:
				printf("cmd fail\n");
				goto error;
		}
	};

	fd_dev = open(dev, O_WRONLY);
	if (fd_dev == -1) {
		printf("can not open %s\n", dev);
		goto error;
	}

	/*************************************
	 *  D4     D3     D2     D1     D0
	 * MODE   LED3   LED2   LED1   LED0
	 *************************************/
	data_buf[LED_MODE_IDX] = led_mode;
	data_buf[LED_3_IDX] = (led_id & 0x04) >> 2;
	data_buf[LED_2_IDX] = (led_id & 0x02) >> 1;
	data_buf[LED_1_IDX] = (led_id & 0x01) >> 0;
	write(fd_dev, data_buf, CMD_BUF_SZ);
	close(fd_dev);

	return 0;

error:
	if (fd_dev != -1)
		close(fd_dev);

	return -1;
}
