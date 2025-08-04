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