/*
 * fat_fuse_ops.c
 *
 * FAT32 filesystem operations for FUSE (Filesystem in Userspace)
 */

#include "fat_fuse_ops.h"

#include "big_brother.h"
#include "fat_file.h"
#include "fat_filename_util.h"
#include "fat_fs_tree.h"
#include "fat_util.h"
#include "fat_volume.h"
#include <alloca.h>
#include <errno.h>
#include <gmodule.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static fat_file fat_tree_search_by_cluster(fat_volume vol, fat_file file,
                                           u32 cluster);
static void print_first_fat_entries(fat_table table);

#define LOG_MESSAGE_SIZE 100
#define DATE_MESSAGE_SIZE 30

static void now_to_str(char *buf) {
    time_t now = time(NULL);
    struct tm *timeinfo;
    timeinfo = localtime(&now);

    strftime(buf, DATE_MESSAGE_SIZE, "%d-%m-%Y %H:%M", timeinfo);
}

static void fat_fuse_log_activity(char *operation_type, fat_file file) {
    struct stat st;
    struct fuse_file_info fi;
    fat_fuse_getattr("/bb/fs.log", &st);
    off_t offset = st.st_size;

    char buf[LOG_MESSAGE_SIZE] = "";
    now_to_str(buf);
    strcat(buf, "\t");
    strcat(buf, getlogin());
    strcat(buf, "\t");
    strcat(buf, file->filepath);
    strcat(buf, "\t");
    strcat(buf, operation_type);
    strcat(buf, "\n");

    fat_fuse_open("/bb/fs.log", &fi);
    fat_fuse_write("/bb/fs.log", buf, strlen(buf), offset, &fi);
    fat_fuse_release("/bb/fs.log", &fi);
}

/* Loads directory children from the volume into in-memory tree.
 * Calls function fat_file_read_children which returns
 * a list of files inside a GList. The children were read from the directory
 * entries in the cluster of the directory. This function iterates over the
 * list of children and adds them to the file tree.
 * This operation should be performed only once per directory, the first time
 * readdir is called.
 */
static void fat_fuse_load_directory_children(fat_volume vol, fat_tree_node dir_node) {
    fat_file dir = fat_tree_get_file(dir_node);
    GList *children_list = fat_file_read_children(dir);

    // Add children to tree. TODO handle duplicates
    for (GList *l = children_list; l != NULL; l = l->next) {
        vol->file_tree =
            fat_tree_insert(vol->file_tree, dir_node, (fat_file)l->data);
    }
}


fat_volume fat_fuse_init(fat_volume vol) {
    fat_file root_dir = fat_file_init_orphan_dir("/", vol->table, vol->root_dir_start_cluster);
    vol->file_tree = fat_tree_init();
    vol->file_tree = fat_tree_insert(vol->file_tree, NULL, root_dir);

    // Load the content of / to the tree
    fat_tree_node root_dir_node = fat_tree_node_search(vol->file_tree, "/");
    fat_fuse_load_directory_children(vol, root_dir_node);

    // Locate or create the Big Brother cluster within the FAT volume
    bool found = false, created = false;
    u32 cluster = bb_find_orphan_cluster(vol);
    if (cluster < MAX_CLUSTERS_TO_SCAN) {
        found = true;
    } else if (cluster == MAX_CLUSTERS_TO_SCAN) {
        cluster = bb_create_cluster(vol);
        created = true;
    }

    // Integrate the Big Brother structure into the in-memory file tree
    if (created || found) {
        // Insert the "/bb" directory
        DEBUG("Inserting \"/bb\" directory into the file tree");
        fat_file bb_dir =
            fat_file_init_orphan_dir(BB_DIRNAME, vol->table, cluster);
        vol->file_tree = fat_tree_insert(vol->file_tree, root_dir_node, bb_dir);
        // Insert the "/bb/fs.log" file
        DEBUG("Inserting \"/bb/fs.log\" file into the file tree");
        fat_tree_node bb_dir_node =
            fat_tree_node_search(vol->file_tree, BB_DIRNAME);
        fat_fuse_load_directory_children(vol, bb_dir_node);
    }

    fat_file result = fat_tree_search_by_cluster(vol, root_dir, 157);
    if (result != NULL) {
        DEBUG("Encontrado: %s", result->filepath);
    } else {
        DEBUG("No se encontró nada en ese cluster");
    }

    print_first_fat_entries(vol->table);

    return vol;
}

/* Get file attributes (file descriptor version) */
int fat_fuse_fgetattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi) {
    fat_file file = (fat_file)fat_tree_get_file((fat_tree_node)fi->fh);
    fat_file_to_stbuf(file, stbuf);
    return 0;
}

/* Load the entire hierarchy of parent directories in the in-memory tree.
 * IMPORTANT: Asumes the rood directory / has been already loaded
 */
static int fat_fuse_load_path(const char *path) {
    fat_volume vol = get_fat_volume();
    size_t len = strlen(path);
    char *prefix = calloc(len + 1, sizeof(char));
    for (size_t i = 1; i <= len; i++) {
        if (path[i] != '/') continue;
        if (path[i] == '\0') break;  // End of path
        // We found a path delimiter /, we take everything up to there
        memcpy(prefix, path, i);
        prefix[i] = '\0';

        fat_tree_node dir_node = fat_tree_node_search(vol->file_tree, prefix);
        if (dir_node == NULL) {
            // This doesn't happen because the parent was added in the
            // previous iteration.
            DEBUG("Directory %s not found in tree\n", prefix);
            errno = ENOENT;
            return -errno;
        }
        fat_file dir = fat_tree_get_file(dir_node);
        if (!fat_file_is_directory(dir)) {
            DEBUG("Directory %s is not a directory\n", prefix);
            errno = ENOTDIR;
            return -errno;
        }
        if (dir->children_read != 1) {
            fat_fuse_load_directory_children(vol, dir_node);
        }
    }
    free(prefix);
    return 0;
}

/* Get file attributes (path version) */
int fat_fuse_getattr(const char *path, struct stat *stbuf) {
    fat_volume vol;
    fat_file file;

    vol = get_fat_volume();

    // Check all parent directories in path are loaded in the tree
    fat_fuse_load_path(path);

    file = fat_tree_search(vol->file_tree, path);
    if (file == NULL) {
        errno = ENOENT;
        return -errno;
    }
    fat_file_to_stbuf(file, stbuf);
    return 0;
}

/* Open a file */
int fat_fuse_open(const char *path, struct fuse_file_info *fi) {
    fat_volume vol;
    fat_tree_node file_node;
    fat_file file;

    vol = get_fat_volume();
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (!file_node)
        return -errno;
    file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file))
        return -EISDIR;
    fat_tree_inc_num_times_opened(file_node);
    fi->fh = (uintptr_t)file_node;
    return 0;
}

/* Open a directory */
int fat_fuse_opendir(const char *path, struct fuse_file_info *fi) {
    fat_volume vol = NULL;
    fat_tree_node file_node = NULL;
    fat_file file = NULL;

    vol = get_fat_volume();
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL) {
        return -errno;
    }
    file = fat_tree_get_file(file_node);
    if (!fat_file_is_directory(file)) {
        return -ENOTDIR;
    }
    fat_tree_inc_num_times_opened(file_node);
    fi->fh = (uintptr_t)file_node;
    return 0;
}

/* Add entries of a directory in @fi to @buf using @filler function. */
int fat_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi) {
    fat_volume vol = get_fat_volume();
    errno = 0;
    fat_tree_node dir_node = (fat_tree_node)fi->fh;
    fat_file dir = fat_tree_get_file(dir_node);
    fat_file *children = NULL, *child = NULL;
    int error = 0;

    // Insert first two filenames (. and ..)
    if ((*filler)(buf, ".", NULL, 0) || (*filler)(buf, "..", NULL, 0)) {
        return -errno;
    }
    if (!fat_file_is_directory(dir)) {
        errno = ENOTDIR;
        return -errno;
    }
    if (dir->children_read != 1) {
        fat_fuse_load_directory_children(vol, dir_node);
        if (errno < 0) {
            return -errno;
        }
    }

    children = fat_tree_flatten_h_children(dir_node);
    child = children;
    while (*child != NULL) {
        if (strcmp((*child)->name, "fs.log") != 0 &&
            strcmp((*child)->name, "bb") != 0) {
            error = (*filler)(buf, (*child)->name, NULL, 0);
            if (error != 0) {
                DEBUG("Error in readdir: %s\n", dir->filepath);
                break;
            }
        }
        child++;
    }
    return 0;
}

/* Read data from a file */
int fat_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    errno = 0;
    int bytes_read;
    fat_tree_node file_node = (fat_tree_node)fi->fh;
    fat_file file = fat_tree_get_file(file_node);
    fat_file parent = fat_tree_get_parent(file_node);

    bytes_read = fat_file_pread(file, buf, size, offset, parent);

    fat_fuse_log_activity("read", file);

    return bytes_read;
}

/* Write data from a file */
int fat_fuse_write(const char *path, const char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {

    int bytes_written;
    fat_tree_node file_node = (fat_tree_node)fi->fh;
    fat_file file = fat_tree_get_file(file_node);
    fat_file parent = fat_tree_get_parent(file_node);

    if (size == 0)
        return 0; // Nothing to write
    if (offset > file->dentry->file_size)
        return -EOVERFLOW;

    bytes_written = fat_file_pwrite(file, buf, size, offset, parent);

    if (strcmp(path, "/bb/fs.log") != 0) {
        fat_fuse_log_activity("write", file);
    }

    return bytes_written;
}

/* Close a file */
int fat_fuse_release(const char *path, struct fuse_file_info *fi) {
    fat_tree_node file = (fat_tree_node)fi->fh;
    fat_tree_dec_num_times_opened(file);
    return 0;
}

/* Close a directory */
int fat_fuse_releasedir(const char *path, struct fuse_file_info *fi) {
    fat_tree_node file = (fat_tree_node)fi->fh;
    fat_tree_dec_num_times_opened(file);
    return 0;
}

int fat_fuse_mkdir(const char *path, mode_t mode) {
    errno = 0;
    fat_volume vol = NULL;
    fat_file parent = NULL, new_file = NULL;
    fat_tree_node parent_node = NULL;

    // The system has already checked the path does not exist. We get the parent
    vol = get_fat_volume();
    parent_node = fat_tree_node_search(vol->file_tree, dirname(strdup(path)));
    if (parent_node == NULL) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_file(parent_node);
    if (!fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }

    // init child
    new_file = fat_file_init(vol->table, true, strdup(path));
    if (errno != 0) {
        return -errno;
    }
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, parent_node, new_file);
    // write file in parent's entry (disk)
    fat_file_dentry_add_child(parent, new_file);
    print_first_fat_entries(vol->table);
    return -errno;
}

/* Creates a new file in @path. @mode and @dev are ignored. */
int fat_fuse_mknod(const char *path, mode_t mode, dev_t dev) {
    errno = 0;
    fat_volume vol;
    fat_file parent, new_file;
    fat_tree_node parent_node;

    // The system has already checked the path does not exist. We get the parent
    vol = get_fat_volume();
    parent_node = fat_tree_node_search(vol->file_tree, dirname(strdup(path)));
    if (parent_node == NULL) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_file(parent_node);
    if (!fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }
    new_file = fat_file_init(vol->table, false, strdup(path));
    if (new_file == NULL) {
        return -errno;
    }
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, parent_node, new_file);
    // Write dentry in parent cluster
    fat_file_dentry_add_child(parent, new_file);
    print_first_fat_entries(vol->table);
    return -errno;
}

int fat_fuse_utime(const char *path, struct utimbuf *buf) {
    errno = 0;
    fat_file parent = NULL;
    fat_volume vol = get_fat_volume();
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_parent(file_node);
    if (parent == NULL || errno != 0) {
        DEBUG("WARNING: Setting time for parent ignored");
        return 0; // We do nothing, no utime for parent
    }
    fat_utime(fat_tree_get_file(file_node), parent, buf);
    return -errno;
}

/* Shortens the file at the given offset.*/
int fat_fuse_truncate(const char *path, off_t offset) {
    errno = 0;
    fat_volume vol = get_fat_volume();
    fat_file file = NULL, parent = NULL;
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }
    file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file))
        return -EISDIR;

    parent = fat_tree_get_parent(file_node);
    fat_tree_inc_num_times_opened(file_node);
    fat_file_truncate(file, offset, parent);
    return -errno;
}

int fat_fuse_unlink(const char *path) {
    errno = 0;
    fat_volume vol = get_fat_volume();

    // Search for the node matching 'path' in the file tree
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL) { // Node not found
        errno = ENOENT;
        return -errno;
    }

    // Get the file associated with the node
    fat_file file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file)) {
        fat_error("Error! File is a directory\n");
        errno = EISDIR;
        return -errno;
    }

    // Get the parent directory of the file
    fat_file parent = fat_tree_get_parent(file_node);

    // Free FAT clusters
    u32 cluster = file->start_cluster;
    u32 next_cluster;
    while (fat_table_cluster_is_valid(cluster)) {
        next_cluster = fat_table_get_next_cluster(file->table, cluster);
        fat_table_set_next_cluster(file->table, cluster, FAT_CLUSTER_FREE);
        cluster = next_cluster;
    }

    // Mark directory entry as deleted
    file->dentry->base_name[0] = FILE_FOR_DELETION;
    write_dir_entry(parent, file);

    // Remove the file node from the file tree
    fat_tree_delete(vol->file_tree, path);

    print_first_fat_entries(vol->table);

    return -errno;
}

int fat_fuse_rmdir(const char *path) {
    errno = 0;
    fat_volume vol = get_fat_volume();

    // Search for the node matching 'path' in the file tree
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL) { // Node not found
        errno = ENOENT;
        return -errno;
    }

    // Get the file associated with the node
    fat_file file = fat_tree_get_file(file_node);
    if (!fat_file_is_directory(file)) {
        fat_error("Error! File is not a directory\n");
        errno = ENOTDIR;
        return -errno;
    }

    // Check if the directory has any child nodes in memory
    if (file->children_read != 1) {
        fat_fuse_load_directory_children(vol, file_node);
    }
    fat_file *children = fat_tree_flatten_h_children(file_node);
    bool has_children = (children[0] != NULL);
    free(children);
    if (has_children) { // Cannot remove a nonempty directory
        fat_error("Error! Directory is not empty\n");
        errno = ENOTEMPTY;
        return -errno;
    }

    // Get the parent directory of the file
    fat_file parent = fat_tree_get_parent(file_node);
    if (parent == NULL) {
        fat_error("Error! Cannot remove root directory\n");
        errno = EPERM;
        return -errno;
    }

    // Free FAT clusters
    u32 cluster = file->start_cluster;
    u32 next_cluster;
    while (fat_table_cluster_is_valid(cluster)) {
        next_cluster = fat_table_get_next_cluster(file->table, cluster);
        fat_table_set_next_cluster(file->table, cluster, FAT_CLUSTER_FREE);
        cluster = next_cluster;
    }

    // Mark directory entry as deleted
    file->dentry->base_name[0] = FILE_FOR_DELETION;
    write_dir_entry(parent, file);

    // Remove the file node from the file tree
    fat_tree_delete(vol->file_tree, path);

    print_first_fat_entries(vol->table);

    return -errno;
}

static void print_chain_of_separator(const char *separator,
                                     unsigned int count) {
    for (unsigned int i = 0; i < count; i++) {
        printf("%s", separator);
    }
    printf("\n");
}

static void print_first_fat_entries(fat_table table) {
    /* FAT entries mapped in memory */
    const le32 *fat_entries = (const le32 *)table->fat_map;

    /* Number of columns per row */
    const u32 cols = 3;

    /* ANSI color codes */
    const char *CLR_RED = "\x1b[31m";
    const char *CLR_YEL = "\x1b[33m";
    const char *CLR_CYAN = "\x1b[36m";
    const char *CLR_RESET = "\x1b[0m";

    /* Ranges of FAT entries to display */
    u32 ranges[][2] = {{0, 8}, {1170, 1175}};

    /* Header separator */
    print_chain_of_separator("=", 95);

    for (int r = 0; r < 2; r++) {
        u32 start = ranges[r][0];
        u32 end = ranges[r][1];

        for (u32 i = start; i <= end; i++) {
            /* Read and decode FAT entry */
            u32 entry = le32_to_cpu(fat_entries[i]);

            /* Print index and raw hex value */
            printf("%4u: 0x%08X ", i, entry);

            char label[32];
            const char *color = NULL;

            /* Choose label and color based on entry type */
            if (entry == FAT_CLUSTER_FREE) {
                snprintf(label, sizeof(label), "(free)");
            } else if (fat_table_cluster_is_EOC(entry)) {
                color = CLR_RED;
                snprintf(label, sizeof(label), "(end)");
            } else if (fat_table_cluster_is_bad_sector(entry)) {
                color = CLR_YEL;
                snprintf(label, sizeof(label), "(bad)");
            } else {
                color = CLR_CYAN;
                snprintf(label, sizeof(label), "(next %u)", entry);
            }

            /* Print label with or without color */
            if (color) {
                printf("%s%-14s%s", color, label, CLR_RESET);
            } else {
                printf("%-14s", label);
            }

            /* Column formatting */
            if ((i - start + 1) % cols == 0) {
                printf("\n");
            } else {
                printf("\t");
            }
        }

        /* Ensure row ends cleanly if last row incomplete */
        if ((end - start + 1) % cols != 0) {
            printf("\n");
        }

        /* Separator between ranges */
        if (r == 0) {
            print_chain_of_separator("-", 95);
        }
    }

    /* Footer separator */
    print_chain_of_separator("=", 95);
}

static fat_file fat_tree_search_by_cluster(fat_volume vol, fat_file file,
                                           u32 cluster) {
    /* CASO BASE 1 */
    if (file->start_cluster == cluster) {
        return file;
    }

    /* CASO BASE 2 */
    if (!fat_file_is_directory(file)) {
        return NULL;
    }

    fat_tree_node file_node =
        fat_tree_node_search(vol->file_tree, file->filepath);

    if (file->children_read == 0) {
        fat_fuse_load_directory_children(vol, file_node);
    }

    fat_file *children = fat_tree_flatten_h_children(file_node);

    /* CASO RECURSIVO */
    for (fat_file *c = children; *c != NULL; c++) {
        fat_file found = fat_tree_search_by_cluster(vol, *c, cluster);
        if (found != NULL) {
            free(children);
            return found;
        }
    }

    free(children);
    return NULL; // no encontrado en este subárbol
}
