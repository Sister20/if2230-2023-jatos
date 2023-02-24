#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/portio.h"

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // Calculate the index into the framebuffer
    uint16_t index = (row * 80 + col) * 2;
    // Set the character and color in the framebuffer
    MEMORY_FRAMEBUFFER[index] = c;
    MEMORY_FRAMEBUFFER[index + 1] = (bg << 4) | fg;
}

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // Calculate the index into the framebuffer
    uint16_t index = r * 80 + c;
    // Send the high byte of the index to the cursor command port
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (index >> 8) & 0xFF);
    // Send the low byte of the index to the cursor command port
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, index & 0xFF);
}

void framebuffer_clear(void) {
    for (uint16_t i = 0; i < 80 * 25 * 2; i += 2) {
        MEMORY_FRAMEBUFFER[i] = 0;
        MEMORY_FRAMEBUFFER[i + 1] = 0x07;
    }
}
