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

const char *dev = "/dev/usb_led0";

void show_help();

void init_data(unsigned char *data, unsigned char c)
{
	unsigned int i;

	for (i = 0; i <= c; i++)
		data[i] = i;
}

int main(int argc, char *argv[])
{
	unsigned char *file_buffer = NULL;
	unsigned char data_buf[10] = {0};
	int fd_dev;

	fd_dev = open(dev, O_WRONLY);
	if (fd_dev == -1) {
		printf("can not open %s\n", dev);
		goto error;
	}

	init_data(data_buf, 50);
	write(fd_dev, data_buf, 10);
	close(fd_dev);

	return 0;

error:
	if (fd_dev != -1)
		close(fd_dev);

	return -1;
}

void show_help()
{
	printf("Usage: usb_led led_id on/off\n");
}
