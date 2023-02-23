#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    framebuffer_clear();
    framebuffer_write(3, 8,  'W', 0, 0xF);
    framebuffer_write(3, 9,  'e', 0, 0xF);
    framebuffer_write(3, 10, 'l', 0, 0xF);
    framebuffer_write(3, 11, 'c', 0, 0xF);
    framebuffer_write(3, 12, 'o', 0, 0xF);
    framebuffer_write(3, 13, 'm', 0, 0xF);
    framebuffer_write(3, 14, 'e', 0, 0xF);
    framebuffer_write(3, 15, ' ', 0, 0xF);
    framebuffer_write(3, 16, 'J', 0, 0xF);
    framebuffer_write(3, 17, 'a', 0, 0xF);
    framebuffer_write(3, 18, 't', 0, 0xF);
    framebuffer_write(3, 19, 'o', 0, 0xF);
    framebuffer_write(3, 20, 's', 0, 0xF);
    framebuffer_set_cursor(3, 10);
    while (TRUE);
}