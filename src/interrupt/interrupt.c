#include "interrupt.h"
#include "idt.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/portio.h"
#include "../keyboard/keyboard.h"
#include "../filesystem/fat32.h"

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

uint8_t len_char(char str[8]){
    uint8_t i = 0;
    while(str[i] != '\0'){
        i++;
    }
    return i;
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA); 
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}


void puts(char *str, uint32_t len, uint8_t color, uint8_t row, uint8_t col) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(row, i + col, str[i], color, 0);
    }
    framebuffer_set_cursor(row, col + len);
}


void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    uint8_t row, col;
    framebuffer_get_cursor(&row, &col);
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);
    }else if (cpu.eax == 1){
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read_directory(request);
    }else if (cpu.eax == 2){
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = write(request);
    }else if (cpu.eax == 3){
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = delete(request);
    }
    else if (cpu.eax == 4) {
        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking());
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char *) cpu.ebx, buf, cpu.ecx);
    } else if (cpu.eax == 5) {
        puts((char *) cpu.ebx, cpu.ecx, cpu.edx, row, col); // Modified puts() on kernel side
    } else if (cpu.eax == 6){
       framebuffer_clear();
       framebuffer_set_cursor(-1, 0);
    } else if (cpu.eax == 7){
        framebuffer_set_cursor(row+1, 0);
    } else if (cpu.eax == 8){
        struct FAT32DirectoryTable dir_table;
        read_clusters(&dir_table, cpu.ebx, 1);
        for(int i=1;i<64;i++){
            if(dir_table.table[i].name[0] != 0){
                puts(dir_table.table[i].name, len_char(dir_table.table[i].name), 0x0F, row+i-1, 0);
            }
        }
    } else if (cpu.eax == 9) {
        if (memcmp((char *) cpu.ebx, "..", 2) == 0) {

        }
        else {
            struct FAT32DirectoryTable dir_table;
            read_clusters(&dir_table, *((int8_t*) cpu.edx), 1);
            uint32_t len = 0;
            while(((char *)cpu.ebx)[len] != '\0') len++;
            int i = 1;
            while(TRUE){
            if(dir_table.table[i].name[0] == '\0') break;
            if (memcmp(dir_table.table[i].name, (char *) cpu.ebx, len) == 0) {
                *((int8_t*) cpu.edx) = (dir_table.table[i].cluster_high << 16 | dir_table.table[i].cluster_low);
                puts("Berhasil pindah direktori ", 26, 0x0F, row, 0);
                puts(dir_table.table[i].name, 8, 0x0F, row, 26);
                break;
            }
            i++;
        }
        }
    }
}

void main_interrupt_handler(struct CPURegister cpu, uint32_t int_number, struct InterruptStack info) {
    // framebuffer_write(1, 0, cpu.eax+48, 0x0F, 0x00);
    // framebuffer_write(20, 0, int_number, 0x0F, 0x00);
    // framebuffer_write(3, 0, 0xE, 0x0F, 0x00);
    // int_number = 0x30;

    switch (int_number) {
        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;
        case 0x0E:
            __asm__("hlt");
            break;
        default:
            break;
    }
}