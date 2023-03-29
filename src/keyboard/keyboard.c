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

void get_last_line_idx(uint8_t* curX, uint8_t* curY) {
  bool flag = FALSE;
  uint8_t i;
  while(!flag) {
    i=80;
    (*curX)--;
    while(MEMORY_FRAMEBUFFER[((*curX) * 80 + i) * 2] == 0 ) {
    i--;
    if (MEMORY_FRAMEBUFFER[((*curX) * 80 + i) * 2] != 0) {
      flag = TRUE;
      break;
    }
    }
  }
  (*curY) = i;
}

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
              if (keyboard_state.buffer_index > 0) {
                  keyboard_state.buffer_index--;
                  framebuffer_write(cursor_x, cursor_y-1, 0, 0xF, 0);
                  cursor_y--;
                  if (MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2] != 0) {
                      
                      for (int i = keyboard_state.buffer_index; i < MAX_KEYBOARD_BUFFER; i++) {
                          char c = keyboard_state.keyboard_buffer[i];
                          if (c != 0) {
                              framebuffer_write(cursor_x, cursor_y, c, 0xF, 0);
                              cursor_y++;
                          }
                          else {
                              break;
                          }
                      }
                  }
              }
          } else if (cursor_x > 0) {
              keyboard_state.buffer_index = 0;
              get_last_line_idx(&cursor_x, &cursor_y);
              framebuffer_write(cursor_x, cursor_y, 0, 0xF, 0);
          }
      }


        // handle enter
        else if (mapped_char == '\n') {
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
            keyboard_state.buffer_index = 0;
            keyboard_state.keyboard_input_on = FALSE;
            framebuffer_write(cursor_x, cursor_y, ' ', 0xF, 0);
            cursor_y = 0;
            cursor_x++;
        }

        //handle normal characters
        else if (mapped_char != 0) {
          // cek apakah ada karakter di posisi kursor saat itu
          if (MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2] != 0) {
            for (int i = 78; i >= cursor_y; i--) {
              MEMORY_FRAMEBUFFER[(cursor_x * 80 + i + 1) * 2] = MEMORY_FRAMEBUFFER[(cursor_x * 80 + i) * 2];
              MEMORY_FRAMEBUFFER[(cursor_x * 80 + i + 1) * 2 + 1] = MEMORY_FRAMEBUFFER[(cursor_x * 80 + i) * 2 + 1];
            }
          }
          // memasukkan karakter baru pada posisi setelah kursor
          MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y + 1) * 2] = mapped_char;
          MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y + 1) * 2 + 1] = 0xF;
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
          keyboard_state.buffer_index++;
          cursor_y++;
          // cek apakah kursor berada di ujung kanan layar
          if (cursor_y == 80) {
            cursor_y = 0;
            cursor_x++;
          }
        }
               
        else if (scancode == 0x48) { // arrow up    
            if (cursor_x > 0) {
                int i;
                for (i = cursor_y; i < 80; i++) {
                    if (MEMORY_FRAMEBUFFER[((cursor_x - 1) * 80 + i) * 2] != 0) {
                        cursor_x--;
                        cursor_y = i;
                        break;
                    }
                }
                if(i == 80){
                  for(i=79; i>=0; i--){
                    if (MEMORY_FRAMEBUFFER[((cursor_x - 1) * 80 + i) * 2] != 0) {
                        cursor_x--;
                        cursor_y = i;
                        break;
                    }
                  }
                }
            }
        }

        else if (scancode == 0x50) { // arrow down
            if (cursor_x < 24) {
              int i;
                for (i = cursor_y; i < 80; i++) {
                    if (MEMORY_FRAMEBUFFER[((cursor_x + 1) * 80 + i) * 2] != 0) {
                        cursor_x++;
                        cursor_y = i;
                        break;
                    }
                }
                if(i == 80){
                  for(i=79; i>=0; i--){
                    if (MEMORY_FRAMEBUFFER[((cursor_x + 1) * 80 + i) * 2] != 0) {
                        cursor_x++;
                        cursor_y = i;
                        break;
                    }
                  }
                }
            }
        
            
        } else if (scancode == 0x4B) { // arrow left
            if (cursor_y > 0) {
                if (cursor_x == 0 && MEMORY_FRAMEBUFFER[cursor_y * 2] != 0) {
                    int i;
                    for (i = 79; i > 0; i--) {
                        if (MEMORY_FRAMEBUFFER[i * 2] != 0) {
                            cursor_y = i;
                            break;
                        }
                    }
                } else {
                    cursor_y--;
                }
            } 
        }

        else if (scancode == 0x4D) { //arrow right
            if (cursor_y < 79) {
                if (MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y + 1) * 2] != 0) {
                    cursor_y++;
                } else if(MEMORY_FRAMEBUFFER[((cursor_x+1) * 80 + 0) * 2] != 0) {
                    cursor_x++;
                    cursor_y = 0;
                }
            }
        }
        // else if (scancode < 128) { // keyboard input
        //     if (cursor_x < 24) {
        //         // move characters to the right of the cursor
        //         for (int i = 79; i > cursor_y; i--) {
        //             MEMORY_FRAMEBUFFER[(cursor_x * 80 + i) * 2] = MEMORY_FRAMEBUFFER[(cursor_x * 80 + i - 1) * 2];
        //             MEMORY_FRAMEBUFFER[(cursor_x * 80 + i) * 2 + 1] = MEMORY_FRAMEBUFFER[(cursor_x * 80 + i - 1) * 2 + 1];
        //         }
        //         // add the new character
        //         MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2] = mapped_char;
        //         MEMORY_FRAMEBUFFER[(cursor_x * 80 + cursor_y) * 2 + 1] = 0xF;
        //         cursor_y++;
        //     }
        // }


      framebuffer_set_cursor(cursor_x, cursor_y);
      
    }
    pic_ack(IRQ_KEYBOARD);
}
