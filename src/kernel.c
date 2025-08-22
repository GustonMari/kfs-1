#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
//#if !defined(__i386__)
//#error "This needs to be compiled with a ix86-elf compiler"
//#endif

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 

// keeps track of the current cursor
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

// shift all the line up
static void terminal_scroll(void) {
    // move all
    // start at 1 because row1 -> row0
	for (size_t y = 1; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
            // position of the row above = position of the actual row
			terminal_buffer[(y - 1) * VGA_WIDTH + x] =
				terminal_buffer[y * VGA_WIDTH + x];
		}
	}
	// clear last line
	for (size_t x = 0; x < VGA_WIDTH; x++) {
		terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
			vga_entry(' ', terminal_color);
	}
    // put the cursor on the last row again
	if (terminal_row > 0) terminal_row--;
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
	if (c == '\n') {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			terminal_scroll();
		}
	} else {
		terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
		if (++terminal_column == VGA_WIDTH) {
			terminal_column = 0;
			if (++terminal_row == VGA_HEIGHT) {
				terminal_scroll();
			}
		}
	}
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

// ! ====================== Multi screen ==========================

#define MAX_SCREENS 3
uint16_t screens[MAX_SCREENS][VGA_WIDTH * VGA_HEIGHT];
int current_screen = 0;

void save_current_screen() {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        screens[current_screen][i] = terminal_buffer[i];
    }
}

void load_screen(int n) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_buffer[i] = screens[n][i];
    }
    current_screen = n;
    terminal_row = 0;
    terminal_column = 0;
}

// ! ====================== Debug functions ==========================

void itoa(int value, char* buffer, int base) {
    // Pointer to fill buffer
    char* current = buffer;

    // Temporary pointers for later reversing
    char* left;
    char* right;

    // Handle negative for base 10
    unsigned int unsigned_value;
    if (value < 0 && base == 10) {
        unsigned_value = -value;
    } else {
        unsigned_value = value;
    }

    // create digits backward, dividing the number by base to extract each digit
    do {
        int digit = unsigned_value % base;
        if (digit < 10) {
            *current = '0' + digit;         // numbers 0–9
        } else {
            *current = 'a' + (digit - 10);  // letters a–f for hex
        }
        current++;
        unsigned_value /= base;             // drop last digit
    } while (unsigned_value > 0);


    if (value < 0 && base == 10) {
        *current = '-';
        current++;
    }

    *current = '\0';

    left = buffer;
    right = current - 1; // \0
    
    // Reverse string
    while (left < right) {
        char temp = *left;
        *left = *right;
        *right = temp;

        left++;
        right--;
    }
}

void vprintf(const char* format, va_list args) {
    char buffer[32]; // temp buffer for numbers

    for (const char* p = format; *p != '\0'; p++) {
        if (*p == '%') {
            p++; // move past '%'
            switch (*p) {
                case 'd': {
                    int value = va_arg(args, int);
                    itoa(value, buffer, 10);
                    terminal_writestring(buffer);
                    break;
                }
                case 'x': {
                    int value = va_arg(args, int);
                    itoa(value, buffer, 16);
                    terminal_writestring(buffer);
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    terminal_writestring(str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    break;
                }
                case '%': {
                    terminal_putchar('%');
                    break;
                }
                default: // unknown specifier
                    terminal_putchar('%');
                    terminal_putchar(*p);
                    break;
            }
        } else {
            terminal_putchar(*p);
        }
    }
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void printk(const char* format, ...) {
    terminal_writestring("[KERNEL] ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// ! ====================== Keyboard controller ==========================

static const char scancode_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', // 0x0E backspace
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',     // 0x1C enter
    0,   // control
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   // left shift
    '\\','z','x','c','v','b','n','m',',','.','/', 0, // right shift
    '*', 0, ' ', // space
    // rest not handled for now
};

// Track whether each key is pressed
static bool key_pressed[128] = {0};

// Track modifier states
static bool shift_pressed = false;
static bool caps_lock = false;

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    // input byte from port into register
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// send End Of Interrupt (EOI) to the PIC.
static inline void outb(uint16_t port, uint8_t val) {
    // so you introduce volatile to prevent caching
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

char keyboard_getchar(void) {
    uint8_t scancode = inb(0x60);

    bool released = scancode & 0x80; // high bit = release
    uint8_t key = scancode & 0x7F;   // remove release bit

    // Update shift keys
    if (key == 0x2A || key == 0x36) { // Left/Right Shift
        shift_pressed = !released;
        return 0;
    }

    // Caps lock toggle
    if (key == 0x3A && !released) {
        caps_lock = !caps_lock;
        return 0;
    }

    // Ignore repeated keys
    if (released) {
        key_pressed[key] = false;
        return 0;
    }

    if (key_pressed[key]) return 0; // already pressed, ignore

    key_pressed[key] = true;

    // Special keys for switching screens
    switch (scancode) {
        case 0x3B: // F1
            save_current_screen();
            load_screen(0);
            return 0;
        case 0x3C: // F2
            save_current_screen();
            load_screen(1);
            return 0;
        case 0x3D: // F3
            save_current_screen();
            load_screen(2);
            return 0;
    }

    char c = scancode_to_ascii[key];

    // Apply shift/caps lock
    if (c >= 'a' && c <= 'z') {
        if (shift_pressed ^ caps_lock) {
            c -= 32; // convert to uppercase
        }
    } else {
        // Handle shifted symbols
        if (shift_pressed) {
            switch (c) {
                case '1': c = '!'; break;
                case '2': c = '@'; break;
                case '3': c = '#'; break;
                case '4': c = '$'; break;
                case '5': c = '%'; break;
                case '6': c = '^'; break;
                case '7': c = '&'; break;
                case '8': c = '*'; break;
                case '9': c = '('; break;
                case '0': c = ')'; break;
                case '-': c = '_'; break;
                case '=': c = '+'; break;
                case '[': c = '{'; break;
                case ']': c = '}'; break;
                case '\\': c = '|'; break;
                case ';': c = ':'; break;
                case '\'': c = '"'; break;
                case ',': c = '<'; break;
                case '.': c = '>'; break;
                case '/': c = '?'; break;
                case '`': c = '~'; break;
            }
        }
    }

    return c;
}

// ! ====================== Main ==========================

void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();

	/* Newline support is left as an exercise. */
	terminal_writestring("42");

    // testing the debug
    printk("i need to test %d if its %s", 42, "Workinng");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
	terminal_writestring("This is white on blue!\n");
    
    while (1)
    {
        char c = 0;
    
        // wait until a key is pressed
        while (!(c = keyboard_getchar()));
    
        terminal_putchar(c);
    }
}
