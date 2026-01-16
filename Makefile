SRCS = kernel.c hw.c graphics.c font.c logo.c screens.c
DRIVER_SRCS = $(wildcard drivers/*.c)
FS_SRCS = $(wildcard fs/*.c)
UI_SRCS = $(wildcard ui/*.c)
FONT_SRCS = $(wildcard fonts/font_inter_*.c)

OBJS = $(SRCS:.c=.o)
DRIVER_OBJS = $(DRIVER_SRCS:.c=.o)
FS_OBJS = $(FS_SRCS:.c=.o)
UI_OBJS = $(UI_SRCS:.c=.o)
FONT_OBJS = $(FONT_SRCS:.c=.o)

ALL_OBJS = $(OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(UI_OBJS) $(FONT_OBJS)

CFLAGS = -Wall -O2 -ffreestanding -nostdlib -mcpu=cortex-a53+nosimd -I. -Ifonts -Idrivers -Ifs -Iui

all: kernel8.img

start.o: start.s
	aarch64-elf-gcc $(CFLAGS) -c start.s -o start.o

%.o: %.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

drivers/%.o: drivers/%.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

fs/%.o: fs/%.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

ui/%.o: ui/%.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

fonts/%.o: fonts/%.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(ALL_OBJS)
	aarch64-elf-ld -nostdlib -T linker.ld start.o $(ALL_OBJS) -o kernel8.elf
	aarch64-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -f kernel8.elf *.o drivers/*.o fs/*.o ui/*.o fonts/*.o *.img

.PHONY: all clean
