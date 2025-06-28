#define _GNU_SOURCE
#include "fs.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#define LOG_FILE "fs.log"

// Global metadata
static Metadata metadata;

// ---- Yardımcı Fonksiyonlar ----

// Metadata'yı diskten okur
int read_metadata(void) {
    int fd = open(DISK_NAME, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t r = pread(fd, &metadata, sizeof(Metadata), 0);
    close(fd);
    return (r == (ssize_t)sizeof(Metadata)) ? 0 : -1;
}

// Metadata'yı diske yazar
int write_metadata(void) {
    int fd = open(DISK_NAME, O_WRONLY);
    if (fd < 0) return -1;
    ssize_t w = pwrite(fd, &metadata, sizeof(Metadata), 0);
    close(fd);
    return (w == (ssize_t)sizeof(Metadata)) ? 0 : -1;
}

// Dosya adının geçerli olup olmadığını kontrol eder
static int valid_filename(const char* filename) {
    if (!filename) return 0;
    size_t len = strlen(filename);
    return (len > 0 && len < FILENAME_MAXLEN);
}

// Metadata içinde dosya indeksini bulur
static int find_file(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (metadata.files[i].used && strcmp(metadata.files[i].name, filename) == 0)
            return i;
    }
    return -1;
}

// Boş file entry bulur
static int find_free_file_entry(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!metadata.files[i].used)
            return i;
    }
    return -1;
}

#define DATA_START METADATA_SIZE

// Loglama fonksiyonu
int fs_log(void) {
    FILE* logf = fopen(LOG_FILE, "a");
    if (!logf) return -1;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(logf, "[%s] Metadate ve operasyonlar işlendi\n", ts);
    fclose(logf);
    return 0;
}

// ---- FileSystem Fonksiyonları ----

// fs_format: disk.sim oluşturur ve metadata sıfırlar
int fs_format(void) {
    int fd = open(DISK_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) return -1;
    if (ftruncate(fd, DISK_SIZE) < 0) { close(fd); return -1; }
    memset(&metadata, 0, sizeof(metadata));
    if (pwrite(fd, &metadata, sizeof(metadata), 0) != (ssize_t)sizeof(metadata)) {
        close(fd); return -1;
    }
    close(fd);
    fs_log();
    return 0;
}

// fs_create: yeni dosya oluşturur
int fs_create(const char* filename) {
    if (!valid_filename(filename)) return -1;
    if (read_metadata() < 0) return -2;
    if (find_file(filename) >= 0) return -3;
    if (metadata.file_count >= MAX_FILES) return -4;
    int idx = find_free_file_entry();
    metadata.files[idx].used = 1;
    strncpy(metadata.files[idx].name, filename, FILENAME_MAXLEN-1);
    metadata.files[idx].size = 0;
    metadata.files[idx].start_block = UINT32_MAX;
    metadata.files[idx].created_time = (uint64_t)time(NULL);
    metadata.file_count++;
    if (write_metadata() < 0) return -5;
    fs_log();
    return 0;
}

// fs_delete: dosyayı siler
int fs_delete(const char* filename) {
    if (read_metadata() < 0) return -1;
    int idx = find_file(filename);
    if (idx < 0) return -2;
    metadata.files[idx].used = 0;
    metadata.file_count--;
    if (write_metadata() < 0) return -3;
    fs_log();
    return 0;
}

// fs_write: overwrite olarak yazar
int fs_write(const char* filename, const char* data, int size) {
    if (!valid_filename(filename) || !data || size <= 0) return -1;
    if (read_metadata() < 0) return -2;
    int idx = find_file(filename);
    if (idx < 0) return -3;

    // Blok hesaplama
    int blocks = (size + BLOCK_SIZE -1) / BLOCK_SIZE;
    // Yeni veri yaz
    int fd = open(DISK_NAME, O_RDWR);
    if (fd < 0) return -4;
    off_t pos = DATA_START + idx * BLOCK_SIZE; // basit sıra
    if (pwrite(fd, data, size, pos) != size) { close(fd); return -5; }
    close(fd);

    metadata.files[idx].start_block = idx; // blok numarası
    metadata.files[idx].size = size;
    if (write_metadata() < 0) return -6;
    fs_log();
    return 0;
}

// fs_read: ofsetten okuyup buffer’a kopyalar
int fs_read(const char* filename, int offset, int size, char* buffer) {
    if (!valid_filename(filename) || !buffer || size <= 0 || offset < 0) return -1;
    if (read_metadata() < 0) return -2;
    int idx = find_file(filename);
    if (idx < 0) return -3;
    if ((uint32_t)(offset + size) > metadata.files[idx].size)
        size = metadata.files[idx].size - offset;
    int fd = open(DISK_NAME, O_RDONLY);
    if (fd < 0) return -4;
    off_t pos = DATA_START + idx * BLOCK_SIZE + offset;
    ssize_t r = pread(fd, buffer, size, pos);
    close(fd);
    return (r < 0) ? -5 : (int)r;
}

// fs_ls: dosya listesini yazdırır
int fs_ls(void) {
    if (read_metadata() < 0) return -1;
    puts("İsim                             Boyut  Oluşturulma");
    puts("---------------------------------------------------------");
    for (int i = 0; i < MAX_FILES; i++) {
        if (metadata.files[i].used) {
            char ts[20];
            time_t t = metadata.files[i].created_time;
            strftime(ts, sizeof(ts), "%F %T", localtime(&t));
            printf("%-32s %8u  %s\n",
                   metadata.files[i].name,
                   metadata.files[i].size,
                   ts);
        }
    }
    return 0;
}

// fs_rename
int fs_rename(const char* old_name, const char* new_name) {
    if (!valid_filename(old_name) || !valid_filename(new_name)) return -1;
    if (read_metadata() < 0) return -2;
    int idx = find_file(old_name);
    if (idx < 0 || find_file(new_name) >= 0) return -3;
    strncpy(metadata.files[idx].name, new_name, FILENAME_MAXLEN-1);
    if (write_metadata() < 0) return -4;
    fs_log();
    return 0;
}

// fs_exists
int fs_exists(const char* filename) {
    if (read_metadata() < 0) return 0;
    return (find_file(filename) >= 0);
}

// fs_size
int fs_size(const char* filename) {
    if (read_metadata() < 0) return -1;
    int idx = find_file(filename);
    return (idx < 0) ? -1 : (int)metadata.files[idx].size;
}

// fs_append
int fs_append(const char* filename, const char* data, int size) {
    if (!valid_filename(filename) || !data || size <= 0) return -1;
    if (read_metadata() < 0) return -2;
    int idx = find_file(filename);
    if (idx < 0) return -3;

    // mevcut veriyi oku
    char* old = malloc(metadata.files[idx].size);
    fs_read(filename, 0, metadata.files[idx].size, old);

    // yeni buffer
    char* buf = malloc(metadata.files[idx].size + size);
    memcpy(buf, old, metadata.files[idx].size);
    memcpy(buf + metadata.files[idx].size, data, size);
    free(old);

    int res = fs_write(filename, buf, metadata.files[idx].size + size);
    free(buf);
    return res;
}

// fs_truncate
int fs_truncate(const char* filename, int new_size) {
    if (!valid_filename(filename) || new_size < 0) return -1;
    if (read_metadata() < 0) return -2;
    int idx = find_file(filename);
    if (idx < 0) return -3;
    if ((uint32_t)new_size > metadata.files[idx].size) return -4;
    metadata.files[idx].size = new_size;
    if (write_metadata() < 0) return -5;
    fs_log();
    return 0;
}

// fs_copy
int fs_copy(const char* src, const char* dst) {
    if (!valid_filename(src) || !valid_filename(dst)) return -1;
    if (read_metadata() < 0) return -2;
    int s = find_file(src);
    if (s < 0) return -3;
    fs_create(dst);
    char* buf = malloc(metadata.files[s].size);
    fs_read(src, 0, metadata.files[s].size, buf);
    int res = fs_write(dst, buf, metadata.files[s].size);
    free(buf);
    return res;
}

// fs_mv
int fs_mv(const char* oldp, const char* newp) {
    return fs_rename(oldp, newp);
}

// fs_defragment (basit: yeniden format + yeniden kopyala)
int fs_defragment(void) {
    // basit defrag: tüm dosyaları bir temp dizide topla, ardından format + yeniden create/write
    Metadata oldm = metadata;
    Metadata backup = metadata;
    char tempbuf[BLOCK_SIZE];
    Metadata tmpmeta = {0};
    // oku
    read_metadata();
    // kopyala
    memcpy(&backup, &metadata, sizeof(Metadata));
    // format
    fs_format();
    // restore her dosya
    for (uint32_t i = 0; i < backup.file_count; i++) {
        if (backup.files[i].used) {
            fs_create(backup.files[i].name);
            fs_write(backup.files[i].name, NULL, 0); // sıfır boyut
            char* buf = malloc(backup.files[i].size);
            fs_read(backup.files[i].name, 0, backup.files[i].size, buf);
            fs_write(backup.files[i].name, buf, backup.files[i].size);
            free(buf);
        }
    }
    fs_log();
    return 0;
}

// fs_check_integrity
int fs_check_integrity(void) {
    if (read_metadata() < 0) return -1;
    if (metadata.file_count > MAX_FILES) return -2;
    for (int i = 0; i < MAX_FILES; i++) {
        if (metadata.files[i].used) {
            if (!valid_filename(metadata.files[i].name)) return -3;
            if (metadata.files[i].size > DISK_SIZE - DATA_START) return -4;
        }
    }
    fs_log();
    return 0;
}

// fs_backup
int fs_backup(const char* backup_filename) {
    int src = open(DISK_NAME, O_RDONLY);
    if (src < 0) return -1;
    int dst = open(backup_filename, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (dst < 0) { close(src); return -2; }
    char buf[4096];
    ssize_t r;
    while ((r = read(src, buf, sizeof(buf))) > 0)
        write(dst, buf, r);
    close(src); close(dst);
    fs_log();
    return 0;
}

// fs_restore
int fs_restore(const char* backup_filename) {
    int src = open(backup_filename, O_RDONLY);
    if (src < 0) return -1;
    int dst = open(DISK_NAME, O_WRONLY|O_TRUNC);
    if (dst < 0) { close(src); return -2; }
    char buf[4096];
    ssize_t r;
    while ((r = read(src, buf, sizeof(buf))) > 0)
        write(dst, buf, r);
    close(src); close(dst);
    fs_log();
    return 0;
}

// fs_cat
int fs_cat(const char* filename) {
    int sz = fs_size(filename);
    if (sz < 0) return -1;
    char* buf = malloc(sz+1);
    int r = fs_read(filename, 0, sz, buf);
    if (r < 0) { free(buf); return -2; }
    buf[r] = '\0';
    puts(buf);
    free(buf);
    return 0;
}

// fs_diff
int fs_diff(const char* f1, const char* f2) {
    int s1 = fs_size(f1), s2 = fs_size(f2);
    if (s1 < 0 || s2 < 0 || s1 != s2) return -1;
    char *b1 = malloc(s1), *b2 = malloc(s2);
    fs_read(f1, 0, s1, b1);
    fs_read(f2, 0, s2, b2);
    int diff = memcmp(b1, b2, s1);
    free(b1); free(b2);
    return diff;
}
    