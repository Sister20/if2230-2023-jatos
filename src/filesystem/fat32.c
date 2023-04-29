#include "../lib-header/stdtype.h"
#include "fat32.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/framebuffer.h"
#define NULL 0
static struct FAT32DriverState driver_state;
static struct FAT32DirectoryTable temp_table;

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

// int8_t read(struct FAT32DriverRequest request) {
//     // Mencari direktori yang terletak pada folder parent request.parent_cluster_number
//     uint32_t current_cluster = request.parent_cluster_number;

//     read_clusters(&driver_state.dir_table_buf, current_cluster, 1);
//     read_clusters(&driver_state.fat_table, 1, 1);

// }

// int8_t read_directory(struct FAT32DriverRequest request) {

// }

int8_t read(struct FAT32DriverRequest request) {
    // Mencari direktori yang terletak pada folder parent request.parent_cluster_number
    uint32_t current_cluster = request.parent_cluster_number;

    read_clusters(&driver_state.dir_table_buf, current_cluster, 1);
    read_clusters(&driver_state.fat_table, 1, 1);

    // Mencari file yang terletak pada folder parent request.parent_cluster_number
    struct FAT32DirectoryEntry *entry = NULL;
    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, sizeof(request.name)) == 0 &&
            memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, sizeof(request.ext)) == 0) {
            entry = &driver_state.dir_table_buf.table[i];
            break;
        }
    }

    if (entry == NULL) {
        return -1;
    }

    if (request.buffer_size < entry->filesize) {
        return -2;
    }

    if (entry->attribute & ATTR_SUBDIRECTORY) {
        return -3;
    }

    uint32_t cluster_number = (entry->cluster_high << 16) | entry->cluster_low;
    uint8_t cluster_count = entry->filesize / CLUSTER_SIZE;
    read_clusters(request.buf, cluster_number, cluster_count);
    return 0;
}

int8_t read_directory(struct FAT32DriverRequest request) {
    // Mencari direktori yang terletak pada folder parent request.parent_cluster_number
    uint32_t current_cluster = request.parent_cluster_number;

    read_clusters(&driver_state.dir_table_buf, current_cluster, 1);
    read_clusters(&driver_state.fat_table, 1, 1);

    // Mencari file yang terletak pada folder parent request.parent_cluster_number
    struct FAT32DirectoryEntry *entry = NULL;
    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, sizeof(request.name)) == 0 &&
            (driver_state.dir_table_buf.table[i].attribute & ATTR_SUBDIRECTORY)) {
            entry = &driver_state.dir_table_buf.table[i];
            break;
        }
    }

    if (entry == NULL) {
        return -1;
    }

    uint32_t cluster_number = (entry->cluster_high << 16) | entry->cluster_low;
    read_clusters(request.buf, cluster_number, 1);

    return 0;
}

int8_t write(struct FAT32DriverRequest request) {
    // Mencari direktori yang terletak pada folder parent request.parent_cluster_number
    uint32_t current_cluster = request.parent_cluster_number;

    read_clusters(&driver_state.dir_table_buf, current_cluster, 1);
    read_clusters(&driver_state.fat_table, 1, 1);

    if(request.name == NULL && request.ext == NULL){
        return -1;
    }

    struct FAT32DirectoryEntry *entry = NULL;
    // Mencari file yang terletak pada folder parent request.parent_cluster_number
    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, sizeof(request.name)) == 0 &&
            memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, sizeof(request.ext)) == 0) {
            entry = &driver_state.dir_table_buf.table[i];
            break;
        }
    }
    if (entry != NULL) {
        return 1;
    }

    // Mencari empty location pada FileAllocationTable
    uint8_t cluster_count = request.buffer_size / CLUSTER_SIZE;
    if(request.buffer_size == 0){
        cluster_count++;
    }
    uint8_t clusters[cluster_count];
    uint8_t count = 0;
    for (uint32_t i = 2; i < CLUSTER_MAP_SIZE; i++) {
        if (driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY && count < cluster_count) {
            clusters[count] = i;
            count++;
        }
        if(count == cluster_count){
            break;
        }
    }

    if (clusters[0] == 0) {
        return -2;
    }
   

    // Menambahkan DirectoryEntry baru dengan nama yang sesuai dari entry.user_attribute = UATTR_NOT_EMPTY
    for (uint32_t i = 1; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        if (driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY) {
            entry = &driver_state.dir_table_buf.table[i];
            memcpy(entry->name, request.name, sizeof(request.name));
            entry->user_attribute = UATTR_NOT_EMPTY;
            entry->cluster_high = clusters[0] >> 16;
            entry->cluster_low = clusters[0] & 0xFFFF;
            entry->filesize = request.buffer_size;

            if (request.buffer_size == 0) {
                entry->attribute = ATTR_SUBDIRECTORY;

                //Masukin sub folder ke parent
                write_clusters(&driver_state.dir_table_buf, current_cluster, 1);
                driver_state.fat_table.cluster_map[clusters[0]] = FAT32_FAT_END_OF_FILE;

                // Bikin directory table baru
                read_clusters(&temp_table, clusters[0], 1);
                init_directory_table(&temp_table,request.name, 1);
                write_clusters(&temp_table, clusters[0], 1);
            } else {
                memcpy(entry->ext, request.ext, 3);

                // Masukin file ke parent
                write_clusters(&driver_state.dir_table_buf, current_cluster, 1);
                
                // Masukin file ke cluster
                struct ClusterBuffer tempBuf;
                for(uint8_t i = 0; i < cluster_count; i++){
                    if(i == cluster_count-1){
                        driver_state.fat_table.cluster_map[clusters[i]] = FAT32_FAT_END_OF_FILE;
                    }else{
                        driver_state.fat_table.cluster_map[clusters[i]] = FAT32_FAT_EMPTY_ENTRY+clusters[i+1];
                    }
                    memcpy(tempBuf.buf, request.buf+(i*CLUSTER_SIZE), CLUSTER_SIZE);  // Kalo pake ini jadi ABCDE
                    // memcpy(tempBuf.buf, request.buf, CLUSTER_SIZE);    // Kalo pake ini jadi A doang
                    write_clusters(&tempBuf, clusters[i], 1);
                }
            }
            write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

            return 0;
        }
    }

    return -1;
}


uint8_t isDirectory(struct FAT32DirectoryEntry *entry){
    if (entry[0].user_attribute != UATTR_NOT_EMPTY || entry[0].filesize != 0){
        return 0;
    }
    else {
        return 1;
    }
}


int8_t delete(struct FAT32DriverRequest request) {
    uint32_t current_cluster = request.parent_cluster_number;
    read_clusters(&driver_state.dir_table_buf, current_cluster, 1);
    read_clusters(&driver_state.fat_table, 1, 1);

    uint32_t fatNumber;
    if(current_cluster == 0){
        return -1;
    }

    for(int i=1;i<64;i++){
        if(memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0){
            if(driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY){
                driver_state.dir_table_buf.table[i].user_attribute = UATTR_EMPTY;
                fatNumber = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
                write_clusters(&driver_state.dir_table_buf, current_cluster, 1);
                read_clusters(&driver_state.dir_table_buf, fatNumber, 1);
                for(int j = 1;j<64;j++){
                    if(driver_state.dir_table_buf.table[j].user_attribute == UATTR_NOT_EMPTY){
                        read_clusters(&driver_state.dir_table_buf, current_cluster, 1);
                        driver_state.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY;
                        write_clusters(&driver_state.dir_table_buf, current_cluster, 1);
                        return -2;
                    }
                }
                break;
            }else{
                if(memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0) {
                    driver_state.dir_table_buf.table[i].user_attribute = UATTR_EMPTY;
                    fatNumber = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
                    write_clusters(&driver_state.dir_table_buf, current_cluster, 1);
                    break;
                }
            }
        }
        if(i==63){
            return -1;
        }
    }

    for(uint32_t i=2;i<64;i++){
        read_clusters(&driver_state.dir_table_buf,i,1);
        if(i==fatNumber){
            if(driver_state.dir_table_buf.table[0].attribute == ATTR_SUBDIRECTORY){
                driver_state.fat_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY;
                write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                return 0;
            }else{
                uint32_t temp = driver_state.fat_table.cluster_map[i];
                uint32_t temp2;
                driver_state.fat_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY;
                write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                if(temp==FAT32_FAT_END_OF_FILE){
                    return 0;
                }else{
                    do{
                        read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                        temp2 = driver_state.fat_table.cluster_map[temp];
                        driver_state.fat_table.cluster_map[temp] = FAT32_FAT_EMPTY_ENTRY;
                        temp = temp2;
                        write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                    }while(temp!=FAT32_FAT_END_OF_FILE);
                    driver_state.fat_table.cluster_map[temp2] = FAT32_FAT_EMPTY_ENTRY;
                    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                    return 0;
                }
            }
        }
    }
    return -1;
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_number*4, cluster_count * CLUSTER_BLOCK_COUNT);
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    uint32_t logical_block_address = cluster_number * CLUSTER_BLOCK_COUNT;
    uint8_t block_count = cluster_count * CLUSTER_BLOCK_COUNT;
    write_blocks(ptr, logical_block_address, block_count);
}

void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        // Read and cache the entire FileAllocationTable into the driver state
        read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    }
}

uint32_t cluster_to_lba(uint32_t cluster) {
    return (cluster - 2) * CLUSTER_BLOCK_COUNT + BOOT_SECTOR + 1;
}

bool is_empty_storage(void) {
    read_blocks(&driver_state.cluster_buf, BOOT_SECTOR, 1);
    return memcmp(driver_state.cluster_buf.buf, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void) {
    // Menulis tanda tangan file system ke sektor boot
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    // Menginisialisasi FileAllocationTable
    memset(&driver_state.fat_table, 0, sizeof(struct FAT32FileAllocationTable));
    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    // Menginisialisasi root directory
    init_directory_table(&driver_state.dir_table_buf, "ROOT_DIR", ROOT_CLUSTER_NUMBER);
    write_clusters(&driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));
    memcpy(dir_table->table[0].name, name, 8);
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].cluster_high = parent_dir_cluster >> 16;
    dir_table->table[0].cluster_low = parent_dir_cluster & 0xFFFF;
}

