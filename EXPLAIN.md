# VM

## Drag and drop

https://docs.oracle.com/en/virtualization/virtualbox/6.0/user/guestadd-dnd.html
https://devcamp.com/trails/development-environments/campsites/virtualbox-tips-tricks/guides/enabling-copy-paste-virtualbox-environment

su -
usermod -aG sudo gugus
reboot

add the disk via command script then reboot

# Links

Bible:
https://wiki.osdev.org/Bare_Bones
Very Very nice documentation and schema:
https://dev.to/yeganegholipour/building-my-first-kernel-understanding-bare-metal-operating-systems-bfg
For keyboard input:
https://wiki.osdev.org/PS/2_Keyboard


## GDT / IDT / PIC 

### PIC
In kernel development, remapping the PIC is a critical step to avoid interrupt conflicts. Here's a detailed explanation:

What is a Vector?
A vector is a number (0-255) that identifies different types of interrupts and exceptions in x86 architecture. When an interrupt occurs, the CPU uses this vector number to look up the appropriate handler in the Interrupt Descriptor Table (IDT).

The Problem:

CPU exceptions (like divide by zero, page fault) use vectors 0-31
By default, the 8259 PIC maps hardware interrupts to vectors 0-15
This creates a conflict where hardware interrupts overlap with CPU exceptions
Default PIC Mapping:

Master PIC: IRQ 0-7 → vectors 0-7
Slave PIC: IRQ 8-15 → vectors 8-15
After Remapping:

Master PIC: IRQ 0-7 → vectors 32-39
Slave PIC: IRQ 8-15 → vectors 40-47
Remapping Process:

Send initialization command words (ICW1-ICW4) to both PICs
Set new vector offsets (32 for master, 40 for slave)
Configure master-slave relationship
Set operation mode (8086 mode)
Code Example:

// Remap master PIC to start at vector 32
outb(0x20, 0x11); // ICW1: Initialize
outb(0x21, 0x20); // ICW2: Vector offset 32
outb(0x21, 0x04); // ICW3: Slave at IRQ2
outb(0x21, 0x01); // ICW4: 8086 mode
Copy
This ensures clean separation between CPU exceptions (0-31) and hardware interrupts (32+).


# Set up cross compiler

Steps to build GCC cross-compiler on Xubuntu for i686-elf
1. Install prerequisite packages
Open your terminal and run:

sudo apt update
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev libcloog-isl-dev
These install tools needed for building GCC and its dependencies.

2. Create a working directory
Make a folder somewhere to keep all the sources and build files:

mkdir ~/src
cd ~/src
3. Download source code for binutils and GCC
You will build binutils first, then GCC.

wget https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz
(Replace versions with latest stable if needed.)

4. Extract the downloaded archives
tar -xf binutils-2.40.tar.xz
tar -xf gcc-13.2.0.tar.xz
5. Build and install binutils
Create a build directory and configure binutils for i686-elf:

mkdir build-binutils
cd build-binutils

../binutils-2.40/configure --target=i686-elf --disable-nls --disable-werror --prefix=/usr/local/cross
make
sudo make install

cd ..
--target=i686-elf: build tools for 32-bit ELF i686 target.

--prefix=/usr/local/cross: install into /usr/local/cross (you can change this if you want).

6. Prepare GCC build
Before building GCC, you need some prerequisites:

cd gcc-13.2.0
./contrib/download_prerequisites
cd ..
This downloads GMP, MPFR, MPC needed by GCC.

7. Build and install GCC (only the compiler, no standard libraries yet)
Make a build directory for GCC and configure it:

mkdir build-gcc
cd build-gcc

# BECAREFULL im very not sure i stop the process make all-gcc because it was very slow i relaunch it with make -j 4 all-gcc has 4 core

../gcc-13.2.0/configure --target=i686-elf --prefix=/usr/local/cross --disable-nls --enable-languages=c --without-headers
# becarefull make it multithreaded here 4 cores
make all-gcc 
sudo make install-gcc
#TODO: need to make other installs that has not been done
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3

cd ..
Notes:

--enable-languages=c: build only C compiler.

--without-headers: no C standard library yet, good for building bare-metal OS kernels.

8. Add cross-compiler to your PATH
To use your cross-compiler easily, add it to your shell PATH:

echo 'export PATH="/usr/local/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
9. Verify your cross-compiler works
Try running:

i686-elf-gcc --version
You should see your newly built compiler info.

## install to build gcc

sudo apt install build-essential
sudo apt install bison
sudo apt install flex
sudo apt install libgmp3-dev
sudo apt install libmpc-dev
sudo apt install libmpfr-dev
sudo apt install texinfo
sudo apt install libisl-dev

# Compile time

i686-elf-as boot.s -o boot.o

i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

then link all objects together

i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc


# Files

# Booting

An ABI is a contract between compiled programs and the system they run on — like the CPU, the operating system, and other programs. It defines how functions are called, how data is laid out in memory, and how to interact with the system at the binary level.

```s
.section .bss
```
This declares that you're working in the .bss section.

.bss is a segment for uninitialized data. It doesn't take up space in the compiled binary, only reserves memory at runtime.

```s
.align 16
```
Ensures that the next data (i.e. the stack) starts at a 16-byte aligned address.

This is required by the System V ABI (used on Linux x86). If it’s not aligned, the behavior of certain instructions becomes undefined (especially SSE, function calls, etc.).

```s
stack_bottom:
```
This is a label marking the bottom of the stack. Not the bottom in memory terms, but rather the start of the reserved space.

Think of this like the "beginning" of the reserved memory for the stack.

```s
.skip 16384
```
This reserves 16 KiB (16,384 bytes) for the stack.

It just moves the memory pointer 16KB forward but doesn’t fill it with data.

Since we’re in .bss, this memory won't take up space in the binary.

```s
stack_top:
```

Memory layout (high address ↓):

  stack_top:     <- esp will point here
      |
  [ 16 KiB stack space reserved here ]
      |
  stack_bottom:  <- not used directly

Stack grows downward from stack_top to stack_bottom


Your custom kernel.ld file tells the linker: "Make _start the entry point." This ensures the binary starts executing at _start.

```s
.section .text
```
Tells the assembler: we’re now placing code in the .text section, which is where executable instructions go.

```s
.global _start
```
Makes _start a global symbol, so the linker and bootloader can see it and jump to it.

```s
.type _start, @function
```
Marks _start as a function (even if written in assembly). Helps with debugging and symbol information.

```s
_start:
```
This is the actual entry point label. Execution will start right here.


this is a pure example:
```s
.section .text
.global _start
.type _start, @function

_start:
    mov esp, stack_top         ; Set up the stack pointer
    call kernel_main           ; Call your main kernel function (written in C)
    cli                        ; Clear interrupts
hang:
    hlt                        ; Halt the CPU
    jmp hang                   ; Infinite loop to avoid returning

```

What is the GDT?
The GDT defines the memory segments the CPU uses for things like code, data, and stack. Even in 32-bit protected mode (which most Multiboot bootloaders use), the CPU expects valid segment descriptors in place.

```s
cli
```
Clear Interrupt Flag: This disables maskable hardware interrupts.

Bootloaders usually already disable interrupts, so this is technically redundant — but it's safe and defensive.

It ensures that hlt won’t be interrupted by timers, I/O, or hardware events.


```s
hlt
```
Halt: The CPU stops executing until the next interrupt is received.

Since interrupts are disabled, this locks up the CPU in a sleep state.

Power usage drops; the CPU does nothing until it’s reset or externally interrupted.


```s
jmp 1b
```
Jump back to label 1 (the hlt instruction).

The b stands for "backward" in GNU assembly: it means "jump to the last label 1 before this line."


# Kernel.c

## Function 1: vga_entry_color
```c
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
    return fg | bg << 4;
}
```
Purpose:
Builds the 1-byte color attribute using foreground and background colors.

How it works:
fg = foreground color (lower 4 bits)

bg << 4 = shift background color into upper 4 bits

fg | (bg << 4) = combine both into a single byte

Example:
```c
vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
```
If:
```
VGA_COLOR_WHITE = 0x0F (binary: 00001111)

VGA_COLOR_BLUE = 0x01 (binary: 00000001)
```
Then:
```
bg << 4 = 00000001 << 4 = 00010000
fg = 00001111
Result = 00011111 = 0x1F
```
This gives white text on a blue background.

## Function 2: vga_entry
```c
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
    return (uint16_t) uc | (uint16_t) color << 8;
}
```
Purpose:
Builds the full 2-byte (16-bit) character entry for the VGA buffer.

How it works:
uc = the ASCII character to display

color = 8-bit color attribute (from vga_entry_color)

color << 8 = move color into high byte

Combine both with bitwise OR:
(char) | (color << 8)

Example:
vga_entry('A', 0x1F);
'A' = 0x41

0x1F << 8 = 0x1F00

0x41 | 0x1F00 = 0x1F41

So you write 0x1F41 to the screen buffer and see white 'A' on blue background.

## Why 0xB8000?
Back in the 1980s, the original IBM PC assigned specific areas of physical memory for different devices. This included video memory, which wasn't part of RAM but was memory-mapped I/O (MMIO), meaning you could read/write it like normal RAM, but it actually talked to hardware (like the video card).

Memory Map Segment (original):
Address	Used For
0x00000	Start of RAM
0xA0000	VGA graphics mode memory (Mode 13h etc.)
0xB0000	Monochrome text mode
0xB8000	Color VGA text mode (Mode 03h)

VGA Text Mode:

Uses memory-mapped I/O at address 0xB8000
Provides 80x25 character display (standard)
Each character uses 2 bytes: one for ASCII character, one for color attributes
Color attribute byte: bits 0-3 (foreground), bits 4-6 (background), bit 7 (blink)


# Linker

TODO: need to make a detailled version of this part