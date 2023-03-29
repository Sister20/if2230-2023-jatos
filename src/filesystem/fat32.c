#include "../lib-header/stdtype.h"
#include "fat32.h"
#include "../lib-header/stdmem.h"
#define NULL 0

static struct FAT32DriverState driver_state;

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

uint32_t cluster_to_lba(uint32_t cluster) {
    return (cluster - 2) * CLUSTER_BLOCK_COUNT + ROOT_CLUSTER_NUMBER;
}

bool is_empty_storage(void) {
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, BOOT_SECTOR, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void) {
    // Write the signature to the boot sector
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    // Write the FAT32 FileAllocationTable
    struct FAT32FileAllocationTable fat_table;
    fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    write_clusters(&fat_table, FAT_CLUSTER_NUMBER, 1);

    // Write the root directory
    struct FAT32DirectoryTable root_dir;
    memset(&root_dir, 0, sizeof(root_dir));
    write_clusters(&root_dir, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        // Read and cache the entire FileAllocationTable into the driver state
        read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    uint32_t lba = cluster_to_lba(cluster_number);
    write_blocks(ptr, lba, cluster_count * CLUSTER_BLOCK_COUNT);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    uint32_t lba = cluster_to_lba(cluster_number);
    read_blocks(ptr, lba, cluster_count * CLUSTER_BLOCK_COUNT);
}

int8_t read_directory(struct FAT32DriverRequest request) {
    if (request.buffer_size != sizeof(struct FAT32DirectoryTable)) {
        return -1;
    }

    struct FAT32DirectoryTable *dir_table = (struct FAT32DirectoryTable *)request.buf;
    read_clusters(dir_table, request.parent_cluster_number, 1);

    return 0;
}

int8_t read(struct FAT32DriverRequest request) {
    // Find the directory entry for the file
    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry *entry = find_directory_entry(&dir_table, request.name, request.ext);
    if (entry == NULL) {
        return 2;
    }

    // Check if the entry is a file
    if (entry->attribute & ATTR_SUBDIRECTORY) {
        return 1;
    }

    // Read the file data
    uint32_t cluster_number = ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;
    uint32_t remaining_size = entry->filesize;
    uint8_t *buf = (uint8_t *)request.buf;
    while (remaining_size > 0 && cluster_number != FAT32_FAT_END_OF_FILE) {
        uint32_t read_size = remaining_size > CLUSTER_SIZE ? CLUSTER_SIZE : remaining_size;
        read_clusters(buf, cluster_number, 1);
        buf += read_size;
        remaining_size -= read_size;
        cluster_number = driver_state.fat_table.cluster_map[cluster_number];
    }

    return 0;
}

int8_t write(struct FAT32DriverRequest request) {
    // Find the directory entry for the file or create a new one if it doesn't exist
    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, request.parent_cluster_number, 1);
    struct FAT32DirectoryEntry *entry = find_directory_entry(&dir_table, request.name, request.ext);
    if (entry == NULL) {
        entry = create_directory_entry(&dir_table, request.name, request.ext);
        write_clusters(&dir_table, request.parent_cluster_number, 1);
    }

    // Write the file data
    uint32_t cluster_number = ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;
    if (cluster_number == 0) {
        cluster_number = allocate_cluster();
        entry->cluster_low = (uint16_t)(cluster_number & 0xFFFF);
        entry->cluster_high = (uint16_t)((cluster_number >> 16) & 0xFFFF);
        write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
        write_clusters(&dir_table, request.parent_cluster_number, 1);
    }
    return 0;
}

struct FAT32DirectoryEntry *find_directory_entry(struct FAT32DirectoryTable *dir_table, char *name, char *ext) {
    for (unsigned int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        struct FAT32DirectoryEntry *entry = &dir_table->table[i];
        if (memcmp(entry->name, name, 8) == 0 && memcmp(entry->ext, ext, 3) == 0) {
            return entry;
        }
    }
    return NULL;
}

struct FAT32DirectoryEntry *create_directory_entry(struct FAT32DirectoryTable *dir_table, char *name, char *ext) {
    for (unsigned int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        struct FAT32DirectoryEntry *entry = &dir_table->table[i];
        if (entry->name[0] == '\0') {
            memcpy(entry->name, name, 8);
            memcpy(entry->ext, ext, 3);
            return entry;
        }
    }
    return NULL;
}

uint32_t allocate_cluster(void) {
    for (unsigned int i = 2; i < CLUSTER_MAP_SIZE; i++) {
        if (driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY) {
            driver_state.fat_table.cluster_map[i] = FAT32_FAT_END_OF_FILE;
            return i;
        }
    }
    return 0;
}

// void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
//     // Clear the directory table
//     memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));

//     // Create the "." entry
//     struct FAT32DirectoryEntry *dot_entry = &dir_table->table[0];
//     memset(dot_entry->name, ' ', 8);
//     memcpy(dot_entry->name, ".", 1);
//     dot_entry->attribute = ATTR_SUBDIRECTORY;
//     dot_entry->cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);
//     dot_entry->cluster_high = (uint16_t)((parent_dir_cluster >> 16) & 0xFFFF);

//     // Create the ".." entry
//     struct FAT32DirectoryEntry *dotdot_entry = &dir_table->table[1];
//     memset(dotdot_entry->name, ' ', 8);
//     memcpy(dotdot_entry->name, "..", 2);
//     dotdot_entry->attribute = ATTR_SUBDIRECTORY;
//     dotdot_entry->cluster_low = (uint16_t)((parent_dir_cluster == ROOT_CLUSTER_NUMBER) ? 0 : (parent_dir_cluster & 0xFFFF));
//     dotdot_entry->cluster_high = (uint16_t)((parent_dir_cluster == ROOT_CLUSTER_NUMBER) ? 0 : ((parent_dir_cluster >> 16) & 0xFFFF));
// }