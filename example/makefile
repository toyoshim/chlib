CFLAGS  = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 --code-size 0xec00 --stack-auto --Werror -Isrc --opt-code-speed
CC      = sdcc
OBJS	  = build/ch559.rel build/gpio.rel build/serial.rel build/timer3.rel
DEFINES	= \
  -DSDA_BIT=P1_0 -DSDA_DIR=P1_DIR -DSDA_PU=P1_PU -DSDA_MASK="(1 << 0)" \
  -DSCL_BIT=P0_1 -DSCL_DIR=P0_DIR -DSCL_PU=P0_PU -DSCL_MASK="(1 << 1)"

.PHONY: all clean build

all: build i2c_device_example.bin i2c_host_example.bin cdc_example.bin \
	hid_example.bin timer3_example.bin
	ch559flasher -w timer3_example.bin

clean:
	rm -rf build *.bin

.SILENT:
build:
	mkdir -p build

build/%.rel: ../src/%.c ../src/*.h ../src/usb/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: ../src/usb/%.c ../src/*.h ../src/usb/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: %.c ../src/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -I../src -o $@ $<

build/i2c_device_example.ihx: $(OBJS) build/i2c.rel build/i2c_device_example.rel
	$(CC) $(CFLAGS) $^ -o $@

build/i2c_host_example.ihx: $(OBJS) build/i2c.rel build/i2c_host_example.rel
	$(CC) $(CFLAGS) $^ -o $@

build/cdc_example.ihx: $(OBJS) build/cdc_device.rel build/cdc_example.rel build/usb_device.rel
	$(CC) $(CFLAGS) $^ -o $@

build/hid_example.ihx: $(OBJS) build/hid_device.rel build/hid_example.rel build/usb_device.rel
	$(CC) $(CFLAGS) $^ -o $@

build/timer3_example.ihx: $(OBJS) build/timer3_example.rel
	$(CC) $(CFLAGS) $^ -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
