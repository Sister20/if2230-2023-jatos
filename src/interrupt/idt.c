#include "idt.h"
#include "../keyboard/keyboard.h"
#include "../filesystem/fat32.h"

// create IDT table
struct IDT interrupt_descriptor_table = {
    .idt_gate = {
      {
        .offset_low = 0,
        .segment = 0,
        ._reserved = 0,
        ._r_bit_1 = 0,
        ._r_bit_2 = 0,
        .gate_32 = 0,
        ._r_bit_3 = 0,
        .privilege = 0,
        .valid_bit = 0,
        .offset_high = 0
      }
    },
    .size = sizeof(struct IDT) - 1
};

struct IDTR _idt_idtr = {
    .limit = sizeof(struct IDT) -1,
    .base = &interrupt_descriptor_table
};

void initialize_idt(void) {
    /* TODO : 
   * Iterate all isr_stub_table,
   * Set all IDT entry with set_interrupt_gate()
   * with following values:
   * Vector: i
   * Handler Address: isr_stub_table[i]
   * Segment: GDT_KERNEL_CODE_SEGMENT_SELECTOR
   * Privilege: 0
   */
   for (int i = 0x30; i < 0x3F; i++) {
       set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 3);
   }
    __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
    __asm__ volatile("sti");
}

void set_interrupt_gate(uint8_t int_vector, void *handler_address, uint16_t gdt_seg_selector, uint8_t privilege) {
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.idt_gate[int_vector];
    // TODO : Set handler offset, privilege & segment
    idt_int_gate->offset_low = (uint32_t)handler_address & 0xFFFF;
    idt_int_gate->offset_high = ((uint32_t)handler_address >> 16) & 0xFFFF;
    idt_int_gate->privilege = privilege;
    idt_int_gate->segment = gdt_seg_selector;
    // Target system 32-bit and flag this as valid interrupt gate
    idt_int_gate->_r_bit_1    = INTERRUPT_GATE_R_BIT_1;
    idt_int_gate->_r_bit_2    = INTERRUPT_GATE_R_BIT_2;
    idt_int_gate->_r_bit_3    = INTERRUPT_GATE_R_BIT_3;
    idt_int_gate->gate_32     = 1;
    idt_int_gate->valid_bit   = 1;
}

// struct TSSEntry _interrupt_tss_entry = {
//     .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
// };

// void main_interrupt_handler(struct CPURegister cpu, uint32_t int_number, struct InterruptStack info) {
//     switch (int_number) {
//         case PIC1_OFFSET + IRQ_KEYBOARD:
//             keyboard_isr();
//             break;
//         case 0x30:
//             syscall(cpu, info);
//             break;
//     }
// }

// void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
//     if (cpu.eax == 0) {
//         struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
//         *((int8_t*) cpu.ecx) = read(request);
//     } else if (cpu.eax == 4) {
//         keyboard_state_activate();
//         __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
//         while (is_keyboard_blocking());
//         char buf[KEYBOARD_BUFFER_SIZE];
//         get_keyboard_buffer(buf);
//         memcpy((char *) cpu.ebx, buf, cpu.ecx);
//     } else if (cpu.eax == 5) {
//         puts((char *) cpu.ebx, cpu.ecx, cpu.edx); // Modified puts() on kernel side
//     }
// }
