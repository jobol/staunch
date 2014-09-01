.PHONY: all

all: staunch-launch staunch-manage

staunch-launch: staunch-launch.c staunch-messages.c staunch-data.c staunch-messages.h staunch-data.h
	$(CC) $(CFLAGS) -DONLY_READ -o $@ staunch-launch.c staunch-messages.c staunch-data.c
	sudo /sbin/setcap cap_setgid=+pe-i $@

staunch-manage: staunch-manage.c staunch-messages.c staunch-data.c itoa.c staunch-messages.h staunch-data.h itoa.h
	$(CC) $(CFLAGS) -o $@ staunch-manage.c staunch-messages.c staunch-data.c itoa.c
	sudo /sbin/setcap cap_sys_admin=+pe-i $@


