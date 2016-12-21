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
#include <stdlib.h>
#include <regex.h>
#include <alloca.h>
#include <dirent.h>
/** using glib over switching languages */
#include <glib.h>

/** the mounted zip archive */
static struct zip* archive;
/** the name of the zip archive */
static char* zip_name;

/** the mounted directory of zip files */
static DIR* zip_dir;
/** the name of the mounted directory of zip files */
static char* zip_dir_name;

/**
 * retrieve the latest archive with the given PATH
 * @param path is the path of the archive
 * @return the pointer to the zip_file, NULL if it doesnt exist
 */
static
struct zip* 
find_latest_archive(const char* path) {
    printf("FINDING LATEST ARCHIVE CONTAINING: %s\n", path);
    int len = strlen(path);
    char folder_path[len + 1];
    if (strlen(path) > 1) {
        strcpy(folder_path, path + 1);
        folder_path[len - 1] = '/';
        folder_path[len] = '\0'; 
    }
    struct zip_stat zipstbuf;
    // NEW!:
    // check each zip-file until u find the latest one
    struct zip* latest_archive = NULL;
    struct dirent* zip_file;
    char[FILENAME_MAX] zip_file_name;

    while ((zip_file = readdir(zip_dir)) != NULL) {
        zip_file_name = zip_file->d_name;
        // make relative path to the zip file
        char[strlen(zip_file_name) + zip_dir_name + 2] fixed_path;
        memset(fixed_path, 0, strlen(fixed_path));
        memcpy(fixed_path, zip_dir_name, strlen(zip_dir_name));
        fixed_path[zip_dir_name] = '/';
        memcpy(fixed_path + strlen(zip_dir_name) + 1, zip_file_name, strlen(zip_file_name));
        printf("FIXED PATH TO ZIP FILE: %s\n", fixed_path);
        // open zip file in dir
        struct zip* temp_archive;
        if (!(temp_archive = zip_open(fixed_path, ZIP_RDONLY, 0))) {
            printf("ERROR OPENING ARCHIVE AT %s\n", fixed_path);
        }
        // find file in temp archive
        if (!zip_stat(temp_archive, folder_path, 0, &zipstbuf) 
                || !zip_stat(temp_archive, path + 1, 0, &zipstbuf)) {
            zip_close(latest_archive);
            latest_archive = temp_archive;
        } else {
            zip_close(temp_archive);
        }
    }
    closedir(zip_dir);
    zip_dir = opendir(zip_dir_name);
    return latest_archive;

}
/** 
 *  retrieves the attributes of a specific file / directory
 *  @param path is the path of the file
 *  @param stbuf is the stat structure to hold the attributes
 *  @return 0 for normal exit status, non-zero otherwise.
 */
static
int 
zipfs_getattr(const char* path, struct stat* stbuf) {
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
    //NEW!:
    // check each zip-file until u find the latest one
    struct zip* latest_archive = find_latest_archive(path);
    // end of NEW!
    if (strlen(path)== 1 || !zip_stat(latest_archive, folder_path, 0, &zipstbuf)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (!zip_stat(latest_archive, path + 1, 0, &zipstbuf)) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = zipstbuf.size;

    } else {
        zip_close(latest_archive);
        return -ENOENT;
    }
    zip_close(latest_archive);
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
static
int
zipfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info* fi) {
    printf("READDIR: %s\n", path);
    // unneeded
    (void) offset;
    (void) fi;

    int numberOfFiles = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);
    //TODO: find the latest zip file for each path
    GArray* added_entries = g_array_new(FALSE, TRUE, sizeof(char*));
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
        // check if the current file is in the directory in the given path

        char* temp_path = strdup(path);
        char* temp_fuse_path = strdup(fuse_name);
        int isInPath = strcmp(dirname(temp_fuse_path), path);
        free(temp_path);
        free(temp_fuse_path);

        // find file in system, add to buffer
        struct stat buffer;
        if (!isInPath && zipfs_getattr(fuse_name, &buffer) == 0) {
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
static
int
zipfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    //unused
    (void) fi;
    (void) offset;
    printf("READ: %s\n", path);

    struct zip_file* file = zip_fopen(archive, path + 1, 0);
    if (!file) {
        printf("%s not found\n", path);
        return -errno;
    }
    // read file
    struct stat stbuf;
    zipfs_getattr(path, &stbuf);
    unsigned int maxSize = stbuf.st_size;
    char tempBuf[maxSize + size + offset];
    memset(tempBuf, 0, maxSize + size + offset);
    int numRead = zip_fread(file, tempBuf, maxSize);
    if (numRead == -1 ) {
        printf("error reading %s\n", path);
    }
    //strcpy(buf, tempBuf + offset);
    memcpy(buf, tempBuf + offset, size);
    zip_fclose(file);

    return size;

}

/**
 * opens the file, checks if the file exists
 * @param path is the relative path to the file
 * @param fi is file info
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_open(const char* path, struct fuse_file_info* fi) {
    printf("OPEN: %s\n", path);
    (void)fi;
    char folder_path[strlen(path)];
    strcpy(folder_path, path + 1);
    folder_path[strlen(path) - 2] = '/';
    folder_path[strlen(path - 1)] = '\0';
    if (zip_name_locate(archive, path + 1, 0) >= 0 
            || zip_name_locate(archive, folder_path, 0)) {
        return 0;
    }
    return -1;
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
static
int
zipfs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    printf("WRITE: %s\n", path);
    (void)fi;
    // read old data
    struct stat stbuf;
    zipfs_getattr(path, &stbuf);
    unsigned int file_size = stbuf.st_size;
    char new_buf[file_size + size + offset];
    memset(new_buf, 0, file_size + size + offset);
    zip_file_t* file =  zip_fopen(archive, path + 1, 0);
    zip_fread(file, new_buf, file_size);
    zip_fclose(file);
    // concat new data into buffer
    memcpy(new_buf + offset, buf, size);

    // write to new file source
    zip_source_t* new_source;
    if ((new_source = zip_source_buffer(archive, new_buf, sizeof(new_buf), 0)) == NULL
            || zip_file_add(archive, path + 1, new_source, ZIP_FL_OVERWRITE) < 0) {
        zip_source_free(new_source);
        printf("error adding new file source %s\n", zip_strerror(archive));
    }
    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);

    return size;
}


/**
 * creates a file
 * @param path is the path of the file
 * @param mode is an attribute of the file
 * @param rdev is another attribute of the file
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_mknod(const char* path, mode_t mode, dev_t rdev) {
    printf("MKNOD: %s\n", path);
    (void)mode;
    (void)rdev;

    // char mt [0];
    zip_source_t* dummy = zip_source_buffer(archive, NULL, 0, 0);
    zip_file_add(archive, path + 1, dummy, ZIP_FL_OVERWRITE);
    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);
    return 0;
}
/**
 * deletes the given file
 * @param path is the path of the file to delete
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_unlink(const char* path) {
    printf("UNLINK: %s\n", path);
    zip_int64_t file_index = zip_name_locate(archive, path + 1, 0);
    zip_delete(archive, file_index);
    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);
    return 0;
}
/**
 * creates a directory with the given name
 * @param path is the name of the directory
 * @param mode is the permissions
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_mkdir(const char* path, mode_t mode) {
    printf("MKDIR: %s\n", path);
    (void)mode;
    zip_dir_add(archive, path + 1, ZIP_FL_ENC_UTF_8);
    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);
    return 0;


}
/**
 * renames the file "from" to "to"
 * @param from is the source
 * @param to is the destination
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_rename(const char* from, const char* to) {
    printf("RENAME: from: %s == to: %s\n", from, to);

    zip_int64_t old_file_index = zip_name_locate(archive, from + 1, 0);
    zip_file_rename(archive, old_file_index, to + 1, 0);

    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);

    return 0;
}
/**
 * extend a given file / shrink it by the given bytes
 * @param path is the path to the file
 * @param size is the size to truncate by
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_truncate(const char* path, off_t size) {
    printf("TRUNCATE: %s\n", path);
    // get size of file
    struct stat file_stats;
    zipfs_getattr(path, &file_stats);
    unsigned int file_size = file_stats.st_size;
    int numZeros = 0;
    if (size >  file_size) {
        numZeros = size - file_size;
    }
    char new_contents[size + numZeros];
    memset(new_contents, 0, size+numZeros);
    zipfs_read(path, new_contents, size, 0, NULL);
    zip_source_t* new_source;
    if ((new_source = zip_source_buffer(archive, new_contents, sizeof(new_contents), 0)) == NULL
            || zip_file_add(archive, path + 1, new_source, ZIP_FL_OVERWRITE) < 0) {
        zip_source_free(new_source);
        printf("error adding new file source %s\n", zip_strerror(archive));
        return -1;
    }
    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);

    return 0;
}
/**
 * check user permissions of a file
 * @param path is the path to the file
 * @param mask is for permissions, unused
 * @return 0 for success, non-zero otherwise
 */
static
int
zipfs_access(const char* path, int mode) {
    printf("ACCESS: %s\n", path);
    (void)mode;
    return zipfs_open(path, NULL);
}

/**
 * get time stamp
 * @param path is the path of object
 * @param ts is the timestamp
 * @param fi is not used
 */
static
int
zipfs_utimens(const char* path,  const struct timespec ts[2]) {
    (void)path;
    (void)ts;
    return 0;
}

/**
 * closes the working zip archive
 */
void
zipfs_destroy(void* private_data) {
    (void)private_data;
    zip_close(archive);
    closedir(zip_dir);
}
/** represents available functionality */
static struct fuse_operations zipfs_operations = {
    .getattr = zipfs_getattr,
    .readdir = zipfs_readdir,
    .read = zipfs_read,
    .mknod = zipfs_mknod, // create file
    .unlink = zipfs_unlink, // delete file
    .mkdir = zipfs_mkdir, // create directory
    .rename = zipfs_rename, // rename a file/directory
    .write = zipfs_write, // write to a file
    .truncate = zipfs_truncate, // truncates file to given size
    .access = zipfs_access, // does file exist?
    .open = zipfs_open, // same as access
    .utimens = zipfs_utimens,
    .destroy = zipfs_destroy,
};

/**
 * The main method of this program
 * calls fuse_main to initialize the filesystem
 * ./file-system <options> <mount point> <zip file> <dir>
 * temporary~!
 */
int
main(int argc, char *argv[]) {
    int* error = NULL;
    zip_dir = opendir(argv[--argc]);
    zip_dir_name = argv[argc];
    zip_name = argv[--argc];
    archive = zip_open(zip_name, 0, error);
    char* newarg[argc];
    for (int i = 0; i < argc; i++) {
        newarg[i] = argv[i];
        printf("%s\n", newarg[i]);
    }

    return fuse_main(argc, newarg, &zipfs_operations, NULL);

}
