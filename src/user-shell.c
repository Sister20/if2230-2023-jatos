#include "lib-header/stdtype.h"
#include "filesystem/fat32.h"
#include "lib-header/stdmem.h"


void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void puts(char *str, uint32_t len, uint8_t color) {
    syscall(5, (uint32_t) str, len, color);
}


int main(void) {
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "ikanaide",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == 0)
        syscall(5, (uint32_t) "owo\n", 4, 0xF);

    char buf[16];
    while (TRUE) {
        puts("jatos@OS-IF2230", 17, 0x2);
        puts(":", 1, 0x8);
        puts("/", 1, 0x1);
        puts("$ ", 2, 0x8);
        syscall(4, (uint32_t) buf, 16, 0);
        syscall(7, 0, 0, 0);
    }

    return 0;
}
