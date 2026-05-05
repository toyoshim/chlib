CFLAGS  = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 --code-size 0xec00 --stack-auto --Werror -Isrc --opt-code-speed
CC      = sdcc
TARGET  = build
DEFINES	= \
  -DSDA_BIT=P1_0 -DSDA_DIR=P1_DIR -DSDA_PU=P1_PU -DSDA_MASK="(1 << 0)" \
  -DSCL_BIT=P0_1 -DSCL_DIR=P0_DIR -DSCL_PU=P0_PU -DSCL_MASK="(1 << 1)"
USB_HID_OBJS = \
	hid.rel hid_dualshock3.rel hid_guncon3.rel hid_keyboard.rel  hid_mouse.rel \
	hid_switch.rel hid_xbox.rel
USB_BLE_OBJS = ble.rel
USB_OBJS = \
  cdc_device.rel hid_device.rel usb_device.rel usb_host.rel \
  $(USB_HID_OBJS) $(USB_BLE_OBJS)
BLE_OBJS = hci.rel l2cap.rel att.rel smp.rel
CRYPTO_OBJS = aes.rel
OBJS	  = \
	adc.rel ch559.rel flash.rel gpio.rel i2c.rel led.rel pwm1.rel serial.rel \
	timer3.rel uart1.rel $(USB_OBJS) $(BLE_OBJS) $(CRYPTO_OBJS)

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

build/%.rel: src/usb/ble/%.c src/*.h src/usb/*.h src/usb/ble/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: src/ble/%.c src/*.h src/ble/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: src/crypto/%.c src/crypto/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/$(TARGET).ihx: $(addprefix build/,$(OBJS))
	$(CC) $(CFLAGS) $(addprefix build/,$(OBJS)) -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
