#include "keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"

static struct KeyboardDriverState keyboard_state;

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


void keyboard_state_activate(void){
    keyboard_state.keyboard_input_on = TRUE;
}

void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
}

void get_keyboard_buffer(char *buf){
    memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

bool is_keyboard_blocking(void){
    return keyboard_state.keyboard_input_on;
}

uint8_t cursor_x=0, cursor_y=0;
void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];

        // TODO : Implement scancode processing
        

        // handle backspace
        if (mapped_char == '\b') {
          if (cursor_y > 0) {

            framebuffer_write(cursor_x, cursor_y-1, 0, 0, 0);
            cursor_y--;
          }
        }

        // handle enter
        else if (mapped_char == '\n') {
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
            keyboard_state.buffer_index = 0;
            keyboard_state.keyboard_input_on = FALSE;
            framebuffer_write(cursor_x, cursor_y, 0, 0, 0);
            cursor_y = 0;
            cursor_x++;
        }

        else if (mapped_char != 0) {
          framebuffer_write(cursor_x, cursor_y, mapped_char, 0, 0xF);
          cursor_y++;
          if (cursor_y == 80) {
            cursor_y = 0;
            cursor_x++;
          }
        }

      framebuffer_set_cursor(cursor_x, cursor_y);
    }
    pic_ack(IRQ_KEYBOARD);
}
