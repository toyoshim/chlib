CFLAGS  = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 --code-size 0xec00 --stack-auto --Werror -Isrc --opt-code-speed
CC      = sdcc
OBJS	  = build/ch559.rel build/gpio.rel build/serial.rel build/timer3.rel
DEFINES	= \
  -DSDA_BIT=P1_0 -DSDA_DIR=P1_DIR -DSDA_PU=P1_PU -DSDA_MASK="(1 << 0)" \
  -DSCL_BIT=P0_1 -DSCL_DIR=P0_DIR -DSCL_PU=P0_PU -DSCL_MASK="(1 << 1)"

.PHONY: all clean build

all: build i2c_example.bin
	ch559flasher -w i2c_example.bin

clean:
	rm -rf build *.bin

.SILENT:
build:
	mkdir -p build

build/%.rel: ../src/%.c ../src/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -o $@ $<

build/%.rel: %.c ../src/*.h
	$(CC) -c $(CFLAGS) $(DEFINES) -I../src -o $@ $<

build/i2c_example.ihx: $(OBJS) build/i2c.rel build/i2c_example.rel
	$(CC) $(CFLAGS) $^ -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
