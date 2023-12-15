CFLAGS  = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 --code-size 0xec00 --stack-auto --Werror -Isrc --opt-code-speed
CC      = sdcc
TARGET  = build
DEFINES	= \
  -DSDA_BIT=P1_0 -DSDA_DIR=P1_DIR -DSDA_PU=P1_PU -DSDA_MASK="(1 << 0)" \
  -DSCL_BIT=P0_1 -DSCL_DIR=P0_DIR -DSCL_PU=P0_PU -DSCL_MASK="(1 << 1)"
USB_OBJS = \
	hid.rel hid_dualshock3.rel hid_guncon3.rel hid_keyboard.rel hid_mouse.rel \
	hid_switch.rel hid_xbox.rel usb_cdc_device.rel usb_device.rel	usb_host.rel
OBJS	  = \
	adc.rel ch559.rel flash.rel gpio.rel i2c.rel led.rel pwm1.rel serial.rel \
	timer3.rel uart1.rel

.PHONY: all clean build

all: build $(TARGET).bin

clean:
	rm -rf build $(TARGET).bin

.SILENT:
build:
	mkdir -p build

build/%.rel: src/%.c src/*.h src/usb/*.h src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: src/usb/%.c src/*.h src/usb/*.h src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: src/usb/hid/%.c src/*.h src/usb/*.h src/usb/hid/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/$(TARGET).ihx: $(addprefix build/,$(OBJS))
	$(CC) $(CFLAGS) $(addprefix build/,$(OBJS)) -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
