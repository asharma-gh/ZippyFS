#ifndef FUSE_OPS_H
#define FUSE_OPS_H
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
#include <wordexp.h>
#include <inttypes.h>
#include <linux/random.h>
#include <limits.h>
#include <zip.h>
#include <syscall.h>
#include <stdlib.h>
#include <sys/time.h>

/***** fuse functions *******/

void zippyfs_init(const char* shdw, const char* zip_dir);

/** retrieves the attributes for thing in path into st */
int zippyfs_getattr(const char *path, struct stat *st);

/** determines if the thing in path has permissions in mask */
int zippyfs_access(const char *path, int mask);

/** reads the directory in path into buf using filler */
int zippyfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

/** creates file in path with the given mode */
int zippyfs_mknod(const char *path, mode_t mode, dev_t rdev);

/** creates directory in path with given mode */
int zippyfs_mkdir(const char *path, mode_t mode);

/** removes a link form the given thing in path */
int zippyfs_unlink(const char *path);

/** removes the directory and its contents */
int zippyfs_rmdir(const char *path);

/** renames the thing from to the thing in to */
int zippyfs_rename(const char *from, const char *to);

/** truncates the thing in path */
int zippyfs_truncate(const char *path, off_t size);

/** determines if the thing in path can be opened */
int zippyfs_open(const char *path, struct fuse_file_info *fi);

/** reads size bytes at offset in the file path into buf */
int zippyfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

/** writes size things  in buf at offset into path */
int zippyfs_write(const char *path, const char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi);

/** does stuff? */
int zippyfs_fsync(const char *path, int isdatasync,
                  struct fuse_file_info *_fi);

/** something about time */
int zippyfs_utimens(const char* path, const struct timespec tv[2]);

/** destroys fs on close */
void zippyfs_destroy(void* private_data);

/** change mode */
int zippyfs_chmod(const char* path, mode_t mode);

/** loads either path or dirname of path to cache */
int load_to_cache(const char* path);

int foobar(const char* ok);

#ifdef __cplusplus
}
#endif
#endif
