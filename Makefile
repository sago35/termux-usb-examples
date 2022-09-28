DEVICE_PATH=`termux-usb -l | gojq -rM '.[0]'`

all: req usbtest
	termux-usb -e ./usbtest $(DEVICE_PATH)

usbtest: main.c
	gcc main.c -lusb-1.0 -o usbtest

req:
	termux-usb -r $(DEVICE_PATH)
