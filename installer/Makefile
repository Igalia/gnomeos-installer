all:
	gcc `pkg-config --cflags gtk+-2.0` -o launch-gnomeos-installer main.c mainwindow.c mainwindow.h `pkg-config --libs gtk+-2.0` -ludev

clean:
	rm launch-gnomeos-installer
