#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include "lib-header/stdtype.h"

#define MEMORY_FRAMEBUFFER ((uint8_t *) 0xB8000)
#define CURSOR_PORT_CMD    0x03D4
#define CURSOR_PORT_DATA   0x03D5

/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at MEMORY_FRAMEBUFFER,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper 4-bit
*/

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */
void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // Calculate the index into the framebuffer
    uint16_t index = (row * 80 + col) * 2;
    // Set the character and color in the framebuffer
    MEMORY_FRAMEBUFFER[index] = c;
    MEMORY_FRAMEBUFFER[index + 1] = (bg << 4) | fg;
}

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
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

/** 
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * 
 */
void framebuffer_clear(void) {
    for (uint16_t i = 0; i < 80 * 25 * 2; i += 2) {
        MEMORY_FRAMEBUFFER[i] = 0;
        MEMORY_FRAMEBUFFER[i + 1] = 0x07;
    }
}

#endif
