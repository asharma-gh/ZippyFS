#ifndef ZIPPYFS_FUSE_OPS_H
#define ZIPPYFS_FUSE_OPS_H

#ifdef __cplusplus
extern "C" {

#endif
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <alloca.h>
#include <limits.h>
#include <sys/time.h>
int zippyfs_init();
int zippyfs_getattr(const char *path, struct stat *st);
int zippyfs_access(const char *path, int mask);
int zippyfs_readlink(const char *path, char *buf, size_t size);
int zippyfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int zippyfs_mknod(const char *path, mode_t mode, dev_t rdev);
int zippyfs_mkdir(const char *path, mode_t mode);
int zippyfs_unlink(const char *path);
int zippyfs_rmdir(const char *path);
int zippyfs_rename(const char *from, const char *to);
int zippyfs_truncate(const char *path, off_t size);
int zippyfs_open(const char *path, struct fuse_file_info *fi);
int zippyfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int zippyfs_write(const char *path, const char *buf, size_t size,
                off_t offset, struct fuse_file_info *fi);
int zippyfs_fsync(const char *path, int isdatasync, 
        struct fuse_file_info *_fi);
int zippyfs_utimens(const char* path, const struct timespec tv[2]);

#ifndef __cplusplus
}

#endif
#endif
