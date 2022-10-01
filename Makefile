DEVICE_PATH=`termux-usb -l | gojq -rM '.[0]'`

OUT = usbtest
CFLAGS = -I./source -I.
SRC = diskio.c source/ff.c source/ffsystem.c source/ffunicode.c main.c
OBJ = $(SRC:%.c=%.o)

ifdef MODE
	CFLAGS += -DMODE=$(MODE)
endif

all: req $(OUT)
	termux-usb -e ./$(OUT) $(DEVICE_PATH)

monitor: FORCE
	termux-usb -e ./u-monitor $(DEVICE_PATH)

touch: FORCE
	termux-usb -e ./u-touch $(DEVICE_PATH)

$(OUT): main.c $(OBJ)
	gcc $(OBJ) -lusb-1.0 -o $(OUT)

.c.o:
	gcc $(CFLAGS) -c $< -o $@

req:
	termux-usb -r $(DEVICE_PATH)

clean:
	-rm -rf $(OBJ) $(OUT)

FORCE:
