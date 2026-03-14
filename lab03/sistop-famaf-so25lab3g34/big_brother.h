#ifndef _BIG_BROTHER_H
#define _BIG_BROTHER_H

#define BB_LOG_FILE "/bb/fs.log"
#define BB_DIRNAME "/bb"
#define LOG_FILE_BASENAME "fs"
#define LOG_FILE_EXTENSION "log"
#define MAX_CLUSTERS_TO_SCAN 10000

#include "fat_file.h"

int bb_is_log_file_dentry(fat_dir_entry dir_entry);

int bb_is_log_filepath(char *filepath);

int bb_is_log_dirpath(char *filepath);

/*
 * Scan the FAT table from the first valid cluster up to
 * MAX_CLUSTERS_TO_SCAN in search of the Big Brother cluster.
 *
 * On success:
 *   - If a cluster is marked as bad and its first directory entry
 *     corresponds to "fs.log", return that cluster number.
 *   - Otherwise, return MAX_CLUSTERS_TO_SCAN.
 *
 * On failure (e.g. memory allocation or I/O error), return
 * MAX_CLUSTERS_TO_SCAN + 1.
 */
u32 bb_find_orphan_cluster(fat_volume vol);

/*
 * Create and initialize the Big Brother cluster within the FAT volume.
 *
 * The function first reserves a new orphan cluster and then creates the
 * "fs.log" directory entry inside it. It returns the cluster number.
 */
u32 bb_create_cluster(fat_volume vol);

#endif
