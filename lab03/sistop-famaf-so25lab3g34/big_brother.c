#include "big_brother.h"
#include "fat_volume.h"
#include "fat_table.h"
#include "fat_util.h"
#include <stdio.h>
#include <string.h>

int bb_is_log_file_dentry(fat_dir_entry dir_entry) {
    return strncmp(LOG_FILE_BASENAME, (char *)(dir_entry->base_name), 2) == 0 &&
           strncmp(LOG_FILE_EXTENSION, (char *)(dir_entry->extension), 3) == 0;
}

int bb_is_log_filepath(char *filepath) {
    return strncmp(BB_LOG_FILE, filepath, 8) == 0;
}

int bb_is_log_dirpath(char *filepath) {
    return strncmp(BB_DIRNAME, filepath, 15) == 0;
}

/*
 * Check whether the first directory entry of the given cluster
 * corresponds to the "fs.log" file.
 */
static bool cluster_contains_fs_log(fat_volume vol, u32 cluster) {
    // Size of a single FAT directory entry (32 bytes)
    size_t dentry_size = sizeof(struct fat_dir_entry_s);

    // Allocate a buffer to hold the first directory entry
    u8 *dentry = calloc(1, dentry_size);
    if (!dentry) {
        DEBUG("Failed to allocate memory for cluster %u", cluster);
        errno = ENOMEM;
        return false;
    }

    // Compute the byte offset of the cluster within the volume
    off_t offset = fat_table_cluster_offset(vol->table, cluster);

    // Read the first directory entry of the cluster into the buffer
    if (full_pread(vol->table->fd, dentry, dentry_size, offset) !=
        dentry_size) {
        DEBUG("Failed to read cluster %u", cluster);
        errno = EIO;
        free(dentry);
        return false;
    }

    // Interpret the buffer as a FAT directory entry and check if it is "fs.log"
    bool res = bb_is_log_file_dentry((fat_dir_entry)dentry);

    free(dentry);
    return res;
}

u32 bb_find_orphan_cluster(fat_volume vol) {
    u32 cluster = vol->root_dir_start_cluster;

    while (cluster < MAX_CLUSTERS_TO_SCAN) {
        u32 fat_entry = fat_table_get_next_cluster(vol->table, cluster);

        if (fat_table_cluster_is_bad_sector(fat_entry)) {
            errno = 0;
            if (cluster_contains_fs_log(vol, cluster)) {
                DEBUG("Found orphan cluster %u with \"fs.log\" as first file",
                      cluster);
                return cluster;
            } else if (errno != 0) {
                DEBUG("Failed to read cluster %u while searching for orphan",
                      cluster);
                return MAX_CLUSTERS_TO_SCAN + 1;
            }
        }

        cluster++;
    }

    DEBUG("The orphan cluster with \"fs.log\" as first file was not found");

    return MAX_CLUSTERS_TO_SCAN;
}

u32 bb_create_cluster(fat_volume vol) {
    // Reserve a new orphan cluster for the Big Brother directory
    u32 bb_cluster = fat_table_get_next_free_cluster(vol->table);
    fat_table_set_next_cluster(vol->table, bb_cluster, FAT_CLUSTER_BAD_SECTOR);
    DEBUG("Created orphan cluster %u", bb_cluster);

    // Reserve a cluster for the "fs.log" file and mark it as EOC
    u32 fslog_cluster = fat_table_get_next_free_cluster(vol->table);
    fat_table_set_next_cluster(vol->table, fslog_cluster,
                               FAT_CLUSTER_END_OF_CHAIN_MIN);

    // Create an auxiliary in-memory directory entry for "fs.log"
    fat_dir_entry fslog =
        fat_file_init_direntry(false, BB_LOG_FILE, fslog_cluster);

    // Write the directory entry to the orphan cluster on disk
    off_t offset = fat_table_cluster_offset(vol->table, bb_cluster);
    full_pwrite(vol->table->fd, fslog, sizeof(struct fat_dir_entry_s), offset);

    free(fslog);

    return bb_cluster;
}
