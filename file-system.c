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
#include <libgen.h>
#include <regex.h>
#include <alloca.h>


/** the mounted zip archive */
static struct zip* archive;




/** 
 *  retrieves the attributes of a specific file / directory
 *  @param path is the path of the file
 *  @param stbuf is the stat structure to hold the attributes
 *  @return 0 for normal exit status, non-zero otherwise.
 */
static int zipfs_getattr(const char* path, struct stat* stbuf) {
    printf("getattr: %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));
    // convert the fuse path to 
    // the possible libzip folder path
    int len = strlen(path);
    char folder_path[len + 1];
    if (strlen(path) > 1) {
        strcpy(folder_path, path + 1);
        folder_path[len - 1] = '/';
        folder_path[len] = '\0'; 
    }
    struct zip_stat zipstbuf;
    printf("===temp folder path: %s\n", folder_path);
    // find folder in archive
    if (strlen(path)== 1 || !zip_stat(archive, folder_path, 0, &zipstbuf)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        printf("**added folder at folder path\n");
    } else {
        // find file in archive
        if (zip_stat(archive, path + 1, 0, &zipstbuf)) {
            printf("path not found: %s\n", path);
            return -errno;
        }
        printf("added file at fuse path + 1\n");
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = zipstbuf.size;
    }

    return 0;
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
static int zipfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info* fi) {
    printf("READDIR: %s\n", path);
    // unneeded
    (void) offset;
    (void) fi;

    int numberOfFiles = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);

    for (int i = 0; i < numberOfFiles; i++) {
        // get a file/directory in the archive
        const char* zip_name = zip_get_name(archive, i, 0); 

        char temp[strlen(zip_name)];
        strcpy(temp, zip_name);
        char* fuse_name;
        // fix libzip path to match the format in fuse
        if (zip_name[strlen(zip_name) - 1] == '/') {
            fuse_name = alloca(strlen(zip_name) + 1);
            temp[strlen(zip_name) - 1] = '\0';
            strcpy(fuse_name + 1, temp);
            fuse_name[0] = '/';
        } else {
            fuse_name = alloca(strlen(zip_name) + 2);
            strcpy(fuse_name + 1, temp);
            fuse_name[0] = '/';
        }

        printf("zip_name: %s\n", zip_name);
        printf("fuse_name: %s\n", fuse_name);
        // find file in system, add to buffer
        struct stat buffer;
        if (zipfs_getattr(fuse_name, &buffer) == 0) {
            printf("added %s\n", fuse_name); 
            filler(buf, basename(fuse_name), NULL, 0);
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
static int zipfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    //unused
    (void) fi;
    (void) offset;
    printf("READ: %s\n", path);
    /** TODO: fix path to be readable by libzip **/
    // open file
    struct zip_file* file = zip_fopen(archive, path, 0);
    if (!file) {
        printf("%s not found\n", path);
        return -errno;
    }
    // read file
    struct stat stbuf;
    zipfs_getattr(path, &stbuf);
    unsigned int maxSize = stbuf.st_size;
    int readAmt;

    if (offset + size > maxSize) {
        readAmt = maxSize;
    } else {
        readAmt = size + offset;
    }
    char tempBuf[readAmt];
    int numRead = zip_fread(file, tempBuf, readAmt);
    if (numRead == -1 ) {
        printf("error reading %s\n", path);
    }
    strcpy(buf, tempBuf + offset);
    zip_fclose(file);

    return numRead - offset;

}

/**
 * opens the file
 * @param path is the relative path to the file
 * @param fi is file info
 * @return 0 for success, non-zero otherwise

 static int zipfs_open(const char* path, struct fuse_file_info* fi) {
 printf("OPEN: %s\n", path);

 return 0;
 }
 */
/**
 * closes the working zip archive
 */
void zipfs_destroy(void* private_data) {
    (void)private_data;
    zip_close(archive);
}
/** represents available functionality */
static struct fuse_operations zipfs_operations = {
    .getattr = zipfs_getattr,
    .readdir = zipfs_readdir,
    .read = zipfs_read,
    .destroy = zipfs_destroy,
};

/**
 * The main method of this program
 * calls fuse_main to initialize the filesystem
 * ./file-system <options> <mount point> <zip file>
 */
int
main(int argc, char *argv[]) {
    int* error = NULL;
    archive = zip_open(argv[--argc], 0, error);
    char* newarg[argc];
    for (int i = 0; i < argc; i++) {
        newarg[i] = argv[i];
        printf("%s\n", newarg[i]);
    }

    return fuse_main(argc, newarg, &zipfs_operations, NULL);

}
