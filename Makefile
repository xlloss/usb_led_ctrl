SUBDIR = usb_led_cmd_tool  usb_led_driver
APP ?= usb_led_cmd

all:
	for i in ${SUBDIR} ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i all || exit $$? ; \
	done

install:
	for i in ${SUBDIR} ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i install || exit $$? ; \
	done

clean:
	for i in ${SUBDIR} ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i clean || exit $$? ; \
	done
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions ${APP}
	
