CFLAGS  = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 --code-size 0xec00 --stack-auto --Werror -Isrc --opt-code-speed
CC      = sdcc
TARGET  = build
OBJS	  = \
	adc.rel ch559.rel flash.rel gpio.rel hid.rel hid_dualshock3.rel \
	hid_guncon3.rel hid_keyboard.rel hid_mouse.rel hid_switch.rel hid_xbox.rel \
	i2c.rel led.rel pwm1.rel serial.rel timer3.rel uart1.rel usb_device.rel	\
	usb_host.rel

.PHONY: all clean build

all: build $(TARGET).bin

clean:
	rm -rf build $(TARGET).bin

.SILENT:
build:
	mkdir -p build

build/%.rel: src/%.c src/*.h
	$(CC) -c $(CFLAGS) -o $@ $<

build/$(TARGET).ihx: $(addprefix build/,$(OBJS))
	$(CC) $(CFLAGS) $(addprefix build/,$(OBJS)) -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
