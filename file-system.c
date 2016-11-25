/**
 * Zipfs: A toy file system using libfuze and libzip
 * @author Arvin Sharma
 * @version 1.0
 */
#define FUSE_USE_VERSION 26
#include <zip.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#define MAX_FILE_SIZE 4096
/** 
 * Returns file attributes
 *  - sets unused fields in stat to 0
 *  - adds file attributes to stat
 * @param path is the path to the file
 * @param stbuf is the stat structure to put the file attributes in
 * @return 0 for normal exit status, non-zero otherwise.
 */



/** returns the attributes of a specific file / directory
 *  @param path is the path of the file
 *  @param stbuf is the stat structure to hold the attributes
 *  @return 0 for normal exit status, non-zero otherwise.
 */
static int zipfs_getattr(const char* path, struct stat* stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
       
    printf("getattr: %s\n", path);
    int res = stat(path, stbuf);
    if (res == -1) {
        return -errno;
    }

    return 0;
}
/**
 * Returns one or more directories to the caller
 * @param path is the path to the directory
 * @param buf is where the directory entries
 * @param filler is a utility function for inserting directory
 * entries into a directory structure, which is buf
 * @param offset is the offset of the next entry or zero
 *
 * @return 0 for normal exit status, non-zero otherwise.
 *
 */
static int zipfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info* fi) {

    // unneeded
    (void) offset;
    (void) fi;
    
    DIR* directoryPath;
    struct dirent* directoryEntry;

        
    directoryPath = opendir(path);
    if (directoryPath == NULL) {
        return -errno;
    }

    while ((directoryEntry = readdir(directoryPath)) != NULL) {
        // construct path of entry
        
        size_t pathLen = strlen(path) + strlen(directoryEntry->d_name);
        char entryPath[pathLen];
        strcpy(entryPath, path);
        strcat(entryPath, directoryEntry->d_name);

        // grab info of entry
        struct stat info;
        zipfs_getattr(entryPath, &info);

        // add to buffer
        if (filler(buf, directoryEntry->d_name, &info, 0)) {
            printf("broke");
            return -errno;
        }
    }

    closedir(directoryPath);

    return 0;

}
/**
 * opens the file at the given path
 *  - just checks for its existence and permission
 * @param path is the path of the file
 * @param fi is the file info
 * @return 0 for success, non-zero otherwise
 */
static int zipfs_open(const char* path, struct fuse_file_info* fi) {
    
    printf("OPEN: %s\n", path);
    int fd = open(path, fi->flags);
    if (fd == -1) {
        return -errno;
    }
    close(fd);

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
static int zipfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    //unused
    (void) fi;

    printf("READ: %s\n", path);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    int numRead = pread(fd, buf, size, offset);
    if (numRead == -1) {
        return -errno;
    }
    close(fd);
    return numRead;
}
/** represents available functionality */
static struct fuse_operations zipfs_operations = {
    .getattr = zipfs_getattr,
    .readdir = zipfs_readdir,
    .open = zipfs_open,
    .read = zipfs_read,
};
/**
 * The main method of this program
 * calls fuse_main to initialize the filesystem
 */
int
main(int argc, char *argv[]) {

    return fuse_main(argc, argv, &zipfs_operations, NULL);

}
