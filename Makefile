# emConOs Makefile

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
BOOT_SRC = $(SRC_DIR)/boot/start.s
KERNEL_SRCS = $(wildcard $(SRC_DIR)/kernel/*.c)
DRIVER_SRCS = $(wildcard $(SRC_DIR)/drivers/*.c)
FS_SRCS = $(wildcard $(SRC_DIR)/fs/*.c)
UI_SRCS = $(wildcard $(SRC_DIR)/ui/*.c)
FONT_SRCS = $(wildcard $(SRC_DIR)/fonts/*.c)

# Object files
BOOT_OBJ = $(BUILD_DIR)/boot/start.o
KERNEL_OBJS = $(patsubst $(SRC_DIR)/kernel/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SRCS))
DRIVER_OBJS = $(patsubst $(SRC_DIR)/drivers/%.c,$(BUILD_DIR)/drivers/%.o,$(DRIVER_SRCS))
FS_OBJS = $(patsubst $(SRC_DIR)/fs/%.c,$(BUILD_DIR)/fs/%.o,$(FS_SRCS))
UI_OBJS = $(patsubst $(SRC_DIR)/ui/%.c,$(BUILD_DIR)/ui/%.o,$(UI_SRCS))
FONT_OBJS = $(patsubst $(SRC_DIR)/fonts/%.c,$(BUILD_DIR)/fonts/%.o,$(FONT_SRCS))

ALL_OBJS = $(BOOT_OBJ) $(KERNEL_OBJS) $(DRIVER_OBJS) $(FS_OBJS) $(UI_OBJS) $(FONT_OBJS)

# Compiler flags
CC = aarch64-elf-gcc
LD = aarch64-elf-ld
OBJCOPY = aarch64-elf-objcopy

CFLAGS = -Wall -O2 -ffreestanding -nostdlib -mcpu=cortex-a53+nosimd -I$(INCLUDE_DIR)

# Targets
all: kernel8.img

# Create build directories
$(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/fs $(BUILD_DIR)/ui $(BUILD_DIR)/fonts:
	mkdir -p $@

# Boot assembly
$(BUILD_DIR)/boot/start.o: $(SRC_DIR)/boot/start.s | $(BUILD_DIR)/boot
	$(CC) $(CFLAGS) -c $< -o $@

# Kernel sources
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/kernel/%.c | $(BUILD_DIR)/kernel
	$(CC) $(CFLAGS) -c $< -o $@

# Driver sources
$(BUILD_DIR)/drivers/%.o: $(SRC_DIR)/drivers/%.c | $(BUILD_DIR)/drivers
	$(CC) $(CFLAGS) -c $< -o $@

# Filesystem sources
$(BUILD_DIR)/fs/%.o: $(SRC_DIR)/fs/%.c | $(BUILD_DIR)/fs
	$(CC) $(CFLAGS) -c $< -o $@

# UI sources
$(BUILD_DIR)/ui/%.o: $(SRC_DIR)/ui/%.c | $(BUILD_DIR)/ui
	$(CC) $(CFLAGS) -c $< -o $@

# Font sources
$(BUILD_DIR)/fonts/%.o: $(SRC_DIR)/fonts/%.c | $(BUILD_DIR)/fonts
	$(CC) $(CFLAGS) -c $< -o $@

# Link and create image
kernel8.img: $(ALL_OBJS)
	$(LD) -nostdlib -T $(SRC_DIR)/linker.ld $(ALL_OBJS) -o $(BUILD_DIR)/kernel8.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel8.elf kernel8.img

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*/*.o $(BUILD_DIR)/kernel8.elf kernel8.img

.PHONY: all clean
