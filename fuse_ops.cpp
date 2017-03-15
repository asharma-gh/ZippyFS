/**
 * ZippFS: A distributed file system in FUSE
 * @author Arvin Sharma
 * @version 4.0
 */
#include "fuse_ops.h"
#include <syscall.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <alloca.h>
#include <limits.h>
#include <cmath>
#include <signal.h>
#include <dirent.h>
#include <wordexp.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <linux/random.h>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <unordered_map>
#include "util.h"
#include "block_cache.h"
#include "block.h"
using namespace std;

BlockCache* block_cache;


/** the path of the mounted directory of zip files */
static char* zip_dir_name;

void
zippyfs_init(const char* zip_dir) {
    zip_dir_name = strdup(zip_dir);
    block_cache = new BlockCache("foo");
}


mutex idx_map_mtx;


/**
 *  retrieves the attributes of a specific file / directory
 *  - returns the attributes of the LATEST file / directory
 *  @param path is the path of the file
 *  @param stbuf is the stat structure to hold the attributes
 *  @return 0 for normal exit status, non-zero otherwise.
 */
int
zippyfs_getattr(const char* path, struct stat* stbuf) {
    printf("getattr: %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));
    if (strlen(path) == 1) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    return block_cache->getattr(path, stbuf);
}

/**
 * Provides contents of a directory
 * @param path is the path to the directory
 * @param buf is where the directory entries
 * @param filler is a utility function for inserting directory
 * entries into a directory structure, which is buf
 * @param offset is the offset of the next entry or zero
 *
 * @return 0 for normal exit status, non-zero otherwise.
 *
 */
int
zippyfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info* fi) {
    printf("READDIR: %s\n", path);
    // unneeded
    (void) offset;
    (void) fi;

    map<string, BlockCache::index_entry> added_names;

    auto vec = block_cache->readdir(path);
    for (auto ent : vec) {
        printf("ADDING %s %llu %dFROM CACHE\n", ent.path.c_str(), ent.added_time, ent.deleted);
        added_names[ent.path] = ent;
    }

    // iterate thru it and add them to filler unless its a deletion
    for (auto entry : added_names) {
        if (!entry.second.deleted) {
            cout << "Adding " << entry.first << endl;
            char* base_name = strdup(entry.first.c_str());
            filler(buf, basename(base_name), NULL, 0);
            free(base_name);
        }
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);


    return 0;
}

/**
 * Reads bytes from the given file into the buffer, starting from an
 * offset number of bytes into the file
 * @param path is the path of the file
 * @param buf is where the output will be returned
 * @param size is the number of bytes to read from the file into the buffer
 * @param offset is the offset of bytes to start reading in the file
 * @param fi has information about the file
 * @return 0 for normal exit status, non-zero otherwise
 */
int
zippyfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    (void) fi;
    (void) offset;
    printf("READ: %s\n", path);
    block_cache->read(path, (uint8_t*)buf, size, offset);
    return size;
}

/**
 * opens the file, checks if the file exists
 * @param path is the relative path to the file
 * @param fi is file info
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_open(const char* path, struct fuse_file_info* fi) {
    printf("OPEN: %s\n", path);
    (void)fi;
    return block_cache->open(path);
}

/**
 * writes bytes to a file at a specified offset
 * @param path is the path to the file
 * @param buf is the bytes to write
 * @param size is the size of the buffer
 * @param offset is the offset in the file
 * @param file info is unused
 * @return the number of bytes written
 */
int
zippyfs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    printf("WRITE to  %s\n",  path);
    (void)fi;
    block_cache->write(path, (uint8_t*)buf, size, offset);
    block_cache->flush_to_shdw(0);
    return size;
}

/**
 * creates a file
 * @param path is the path of the file
 * @param mode is an attribute of the file
 * @param rdev is another attribute of the file
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_mknod(const char* path, mode_t mode, dev_t rdev) {
    printf("MKNOD: %s\n", path);
    (void)rdev;
    return block_cache->make_file(path, mode, 1);
}

/**
 * deletes the given file
 * @param path is the path of the file to delete
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_unlink(const char* path) {
    printf("UNLINK: %s\n", path);
    return block_cache->remove(path);
}

/**
 * deletes the given directory
 * @param path is the path to the directory
 * assumes it is empty
 */
int
zippyfs_rmdir(const char* path) {
    printf("RMDIR: %s\n", path);
    return block_cache->rmdir(path);
}
/**
 * creates a directory with the given name
 * @param path is the name of the directory
 * @param mode is the permissions
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_mkdir(const char* path, mode_t mode) {
    printf("MKDIR: %s\n", path);
    return block_cache->make_file(path, mode | S_IFDIR, 1);

}
/**
 * renames the file "from" to "to"
 * @param from is the source
 * @param to is the destination
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_rename(const char* from, const char* to) {
    printf("RENAME \n");
    return block_cache->rename(from, to);
}

/**
 * extend a given file / shrink it by the given bytes
 * @param path is the path to the file
 * @param size is the size to truncate by
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_truncate(const char* path, off_t size) {
    printf("TRUNCATE: %s\n", path);
    block_cache->truncate(path, size);
    block_cache->flush_to_shdw(0);
    return 0;
}

/**
 * TODO:
 * check user permissions of a file
 * @param path is the path to the file
 * @param mask is for permissions, unus ed
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_access(const char* path, int mode) {
    printf("ACCESS: %s %d\n", path, mode);
    (void)mode;
    return 0;

}

/**
 * TODO:
 * changes permissions of thing in path
 */
int
zippyfs_chmod(const char* path, mode_t  mode) {
    (void)path;
    (void)mode;
    return 0;
}

/**
 * symlink
 */
int
zippyfs_symlink(const char* from, const char* to) {
    return block_cache->symlink(from, to);
}

/**
 * read symlink
 */
int
zippyfs_readlink(const char* path, char* buf, size_t size) {
    return block_cache->readlink(path, (uint8_t*)buf, size);
}

/**
 * stub
 */
int
zippyfs_utimens(const char* path,  const struct timespec ts[2]) {
    (void)path;
    (void)ts;
    return 0;
}
/**
 * closes the working zip archive
 */
void
zippyfs_destroy(void* private_data) {
    (void)private_data;
    // flush
    block_cache->flush_to_shdw(1);
    free(zip_dir_name);
}
