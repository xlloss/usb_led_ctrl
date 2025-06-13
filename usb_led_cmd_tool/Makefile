APP ?= usb_led_cmd
OBJ = usb_led_cmd.o
SRC = usb_led_cmd.c
CC = ${CROSS_COMPILE}gcc

all: ${SRC}
	${CC} -o ${APP} ${SRC}

clean:
	rm -f ${APP}   *~ *.o
