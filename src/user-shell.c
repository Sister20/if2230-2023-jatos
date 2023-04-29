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

void read_buf(uint32_t buf){
    if(memcmp((char*)buf, "cd ", 3) == 0){
        syscall(5, (uint32_t)"cd", 2, 0xF);
    }else if(memcmp((char*)buf, "ls ", 3) == 0 || memcmp((char*)buf, "ls", 2) == 0){
        syscall(8, 0, 0, 0);
    }else if(memcmp((char*)buf, "mkdir ", 6) == 0){
        syscall(5, (uint32_t)"mkdir", 6, 0xF);
    }else if(memcmp((char*)buf, "cat ", 4) == 0){
        syscall(5, (uint32_t)"cat", 3, 0xF);
    }else if(memcmp((char*)buf, "cp ", 3) == 0){
        syscall(5, (uint32_t)"cp", 2, 0xF);
    }else if(memcmp((char*)buf, "rm ", 3) == 0){
        syscall(5, (uint32_t)"rm", 2, 0xF);
    }else if(memcmp((char*)buf, "mv ", 3) == 0){
        syscall(5, (uint32_t)"mv", 2, 0xF);
    }else if(memcmp((char*)buf, "whereis ", 8) == 0){
        syscall(5, (uint32_t)"whereis", 8, 0xF);
    }else if(memcmp((char*)buf, "clear", 5) == 0 || memcmp((char*)buf, "clear ", 6) == 0){
        syscall(6, 0, 0, 0);
    }else{
        syscall(5, (uint32_t) buf,10 , 0xF);
        syscall(5, (uint32_t)": command not found", 20, 0xF);
    }
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
        syscall(5, (uint32_t) "owo", 4, 0xF);
    while (TRUE) {
        // syscall(8, 0, 0, 0);
        syscall(5, (uint32_t)"jatos@OS-IF2230", 15, 0xD);
        syscall(5, (uint32_t)":", 1, 0xF);
        syscall(5, (uint32_t)"/root", 5, 0xB);
        syscall(5, (uint32_t)"$ ", 2, 0xF);
        char buf[16];
        syscall(4, (uint32_t) buf, 16, 0x0);
        read_buf((uint32_t) buf);
        // syscall(5, (uint32_t) buf, 16, 0xF);
        syscall(7, 0, 0, 0);
        // syscall(8, 0, 0, 0);
    }

    return 0;
}
