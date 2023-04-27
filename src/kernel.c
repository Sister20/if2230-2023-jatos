#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "interrupt/interrupt.h"
#include "interrupt/idt.h"
#include "keyboard/keyboard.h"
#include "filesystem/fat32.h"
#include "paging/paging.h"

// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     framebuffer_clear();
//     framebuffer_write(3, 8,  'W', 0, 0xF);
//     framebuffer_write(3, 9,  'e', 0, 0xF);
//     framebuffer_write(3, 10, 'l', 0, 0xF);
//     framebuffer_write(3, 11, 'c', 0, 0xF);
//     framebuffer_write(3, 12, 'o', 0, 0xF);
//     framebuffer_write(3, 13, 'm', 0, 0xF);
//     framebuffer_write(3, 14, 'e', 0, 0xF);
//     framebuffer_write(3, 15, ' ', 0, 0xF);
//     framebuffer_write(3, 16, 'J', 0, 0xF);
//     framebuffer_write(3, 17, 'a', 0, 0xF);
//     framebuffer_write(3, 18, 't', 0, 0xF);
//     framebuffer_write(3, 19, 'o', 0, 0xF);
//     framebuffer_write(3, 20, 's', 0, 0xF);
//     framebuffer_set_cursor(3, 20);
//     while (TRUE);
// }

// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     activate_keyboard_interrupt();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     while (TRUE) 
//       keyboard_state_activate();

// }

// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();
//     keyboard_state_activate();

//     struct ClusterBuffer cbuf[5];
//     for (uint32_t i = 0; i < 5; i++)
//         for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
//             cbuf[i].buf[j] = i + 'a';

//     struct FAT32DriverRequest request = {
//         .buf                   = cbuf,
//         .name                  = "ikanaide",
//         .ext                   = "uwu",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = 0,
//     } ;

//     write(request);  // Create folder "ikanaide"

//     memcpy(request.name, "kano1\0\0", 8);
//     write(request);  // Create folder "kano1"
//     // memcpy(request.name, "subfold", 8);
//     // request.parent_cluster_number = ROOT_CLUSTER_NUMBER+1;
//     // write(request);  // Create fragmented file "daijoubu"
//     // request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
//     memcpy(request.name, "ikanaide", 8);
//     delete(request); // Delete first folder, thus creating hole in FS
//     memcpy(request.name, "daijoubu", 8);
//     request.buffer_size = 3*CLUSTER_SIZE;
//     write(request);  // Create fragmented file "daijoubu"
//     delete(request); // Delete daijobu
//     memcpy(request.name, "alpaaaa", 8);
//     request.buffer_size = CLUSTER_SIZE;
//     write(request);  // Create fragmented file "daijoubu"


//     // struct ClusterBuffer readcbuf;
//     // read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
//     // // If read properly, readcbuf should filled with 'a'

//     // request.buffer_size = CLUSTER_SIZE;
//     // read(request);   // Failed read due not enough buffer size
//     // request.buffer_size = 5*CLUSTER_SIZE;
//     // read(request);   // Success read on file "daijoubu"

//     while (TRUE);
// }

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    keyboard_state_activate();
    gdt_install_tss();
    set_tss_register();

    // // Allocate first 4 MiB virtual memory
    // allocate_single_user_page_frame((uint8_t*) 0);

    // // Write shell into memory
    // struct FAT32DriverRequest request = {
    //     .buf                   = (uint8_t*) 0,
    //     .name                  = "shell",
    //     .ext                   = "\0\0\0",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 0x100000,
    // };
    // read(request);

    // // Set TSS $esp pointer and jump into shell 
    // set_tss_kernel_current_stack();
    // kernel_execute_user_program((uint8_t*) 0);

    while (TRUE);
}
