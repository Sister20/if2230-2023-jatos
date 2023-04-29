#include "keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"

static struct KeyboardDriverState keyboard_state;
static uint8_t cursor_x=0, cursor_y=0;
#define MAX_KEYBOARD_BUFFER 256

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

void keyboard_state_activate(void){ // mengaktifkan keyboard
    keyboard_state.keyboard_input_on = TRUE;
}

void keyboard_state_deactivate(void){ // menonaktifkan keyboard
    keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(char *buf){ // mengambil buffer keyboard
    memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

bool is_keyboard_blocking(void){ // mengecek apakah keyboard sedang diblok
    return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void) {
    framebuffer_get_cursor(&cursor_x, &cursor_y);
    if (!is_keyboard_blocking()) {
        keyboard_state.buffer_index = 0;
    } else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];

        if (mapped_char == '\b') {
            if (cursor_y > 0) {
                keyboard_state.buffer_index--;
                cursor_y--;
                if (MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2] != 0) {
                    for (int i = cursor_y; i < 79; i++) {
                        char c = MEMORY_FRAMEBUFFER[(cursor_x * 80 + i + 1) * 2];
                        if (c != 0) {
                            framebuffer_write(cursor_x, i, c, 0xF, 0);
                            framebuffer_write(cursor_x, i+1, 0, 0xF, 0);
                        } else {
                            break;
                        }
                    }
                    framebuffer_write(cursor_x, cursor_y, 0, 0xF, 0);
                } else {
                    framebuffer_write(cursor_x, cursor_y, 0, 0xF, 0);
                }
            }
            else if (cursor_x > 0) {
                keyboard_state.buffer_index = 0;
                if (MEMORY_FRAMEBUFFER[((cursor_x-1)*80 + 79)*2] != 0) {
                    for (int i = 0; i < 80; i++) {
                        char c = MEMORY_FRAMEBUFFER[((cursor_x)*80 + i)*2];
                        if (c != 0) {
                            framebuffer_write(cursor_x-1, i, c, 0xF, 0);
                            framebuffer_write(cursor_x, i, 0, 0xF, 0);
                        } else {
                            break;
                        }
                    }
                    cursor_x--;
                    cursor_y = 79;
                } else {
                    framebuffer_write(cursor_x, cursor_y, 0, 0xF, 0);
                    cursor_x--;
                    cursor_y = 79;
                }
            }
        }

        // handle enter
        else if (mapped_char == '\n') {
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
            keyboard_state.buffer_index = 0;
            keyboard_state.keyboard_input_on = FALSE;
            // newline
            cursor_x++;
            cursor_y = 0;
        }

        //handle normal characters
        else if (mapped_char != 0) {
            MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2] = mapped_char;
            MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2 + 1] = 0xF;
            cursor_y++;
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
            keyboard_state.buffer_index++;
        }  
        else if (scancode == EXT_SCANCODE_LEFT) { // arrow left
            if (cursor_y > 0) {
               if(MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y - 1) * 2] != 0) {
                    cursor_y--;
               } else if(MEMORY_FRAMEBUFFER[((cursor_x-1) * 80 + 79) * 2] != 0) {
                    cursor_x--;
                    cursor_y = 79;
               }
            } 
        }
        else if (scancode == EXT_SCANCODE_RIGHT) { //arrow right
            if (cursor_y < 79) {
                if (MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y + 1) * 2] != 0) {
                    cursor_y++;
                } else if(MEMORY_FRAMEBUFFER[((cursor_x+1) * 80 + 0) * 2] != 0) {
                    cursor_x++;
                    cursor_y = 0;
                }
            }
        }
        framebuffer_set_cursor(cursor_x, cursor_y);  
    }
    pic_ack(IRQ_KEYBOARD);
}
