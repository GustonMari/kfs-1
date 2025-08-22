# Cross compiler (must be installed first)
CC = i686-elf-gcc
AS = i686-elf-as

# Compilation flags (from subject)
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
         -fno-builtin -fno-stack-protector \
         -nostdlib -nodefaultlibs

LDFLAGS = -T src/linker.ld -nostdlib

# Source folder
SRC = src

# Files
OBJS = $(SRC)/boot.o $(SRC)/kernel.o

# Output kernel
KERNEL = gmary_ndormoy_os.bin

all: $(KERNEL)

$(SRC)/boot.o: $(SRC)/boot.s
	$(AS) $(SRC)/boot.s -o $(SRC)/boot.o

$(SRC)/kernel.o: $(SRC)/kernel.c
	$(CC) -c $(SRC)/kernel.c -o $(SRC)/kernel.o $(CFLAGS)

$(KERNEL): $(OBJS) $(SRC)/linker.ld
	$(CC) $(LDFLAGS) -o $(KERNEL) -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

# Verify if kernel is Multiboot compliant
verify: $(KERNEL)
	@if grub-file --is-x86-multiboot $(KERNEL); then \
		echo "Multiboot confirmed"; \
	else \
		echo "The file is not Multiboot!"; \
		exit 1; \
	fi

# Create an ISO with GRUB so QEMU/VirtualBox can boot it
iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "gmary_ndormoy_os" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/$(KERNEL)' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o gmary_ndormoy_os.iso iso

# Run in QEMU
run: iso
	qemu-system-i386 -cdrom gmary_ndormoy_os.iso

# Clean
clean:
	rm -f $(OBJS) $(KERNEL)
	rm -rf iso gmary_ndormoy_os.iso
