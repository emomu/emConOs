SRCS = kernel.c hw.c graphics.c font.c logo.c screens.c
FONT_SRCS = $(wildcard fonts/font_inter_*.c)
OBJS = $(SRCS:.c=.o)
FONT_OBJS = $(FONT_SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -mcpu=cortex-a53+nosimd -I. -Ifonts

all: kernel8.img

start.o: start.s
	aarch64-elf-gcc $(CFLAGS) -c start.s -o start.o

%.o: %.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

fonts/%.o: fonts/%.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS) $(FONT_OBJS)
	aarch64-elf-ld -nostdlib -T linker.ld start.o $(OBJS) $(FONT_OBJS) -o kernel8.elf
	aarch64-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -f kernel8.elf *.o fonts/*.o *.img
