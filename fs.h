#ifndef FS_H
#define FS_H

#include <stdint.h>

#define DISK_NAME       "disk.sim"
#define DISK_SIZE       (1024 * 1024)    // 1 MB
#define METADATA_SIZE   (4 * 1024)       // 4 KB
#define BLOCK_SIZE      512
#define MAX_FILES       64
#define FILENAME_MAXLEN 32

typedef struct {
    char     name[FILENAME_MAXLEN];
    uint32_t size;
    uint32_t start_block;
    uint64_t created_time;  // Unix timestamp
    uint8_t  used;          // 1 if file slot is used
} FileEntry;

typedef struct {
    uint32_t   file_count;
    FileEntry  files[MAX_FILES];
} Metadata;

// Metadata okuma/yazma (main.c tarafından da çağrılır)
int read_metadata(void);
int write_metadata(void);

// FileSystem operasyonları
int fs_format(void);
int fs_create(const char* filename);
int fs_delete(const char* filename);
int fs_write(const char* filename, const char* data, int size);
int fs_read(const char* filename, int offset, int size, char* buffer);
int fs_ls(void);
int fs_rename(const char* old_name, const char* new_name);
int fs_exists(const char* filename);
int fs_size(const char* filename);
int fs_append(const char* filename, const char* data, int size);
int fs_truncate(const char* filename, int new_size);
int fs_copy(const char* src_filename, const char* dest_filename);
int fs_mv(const char* old_path, const char* new_path);
int fs_defragment(void);
int fs_check_integrity(void);
int fs_backup(const char* backup_filename);
int fs_restore(const char* backup_filename);
int fs_cat(const char* filename);
int fs_diff(const char* file1, const char* file2);
int fs_log(void);

#endif // FS_H