/**
 * Zipfs: A toy distributed file system using libfuze and libzip
 * @author Arvin Sharma
 * @version 2.0
 */
#define FUSE_USE_VERSION 26
#include <syscall.h>
#include <zip.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <alloca.h>
#include <limits.h>
#include <dirent.h>
#include <wordexp.h>
#include <linux/random.h>
/** using glib over switching languages */
#include <glib.h>

/** the mounted zip archive */
static struct zip* archive;
/** the name of the zip archive */
static char* zip_name;



/** the name of the mounted directory of zip files */
static char* zip_dir_name;

/** cache for writes */
//static struct zip* cached_writes;
static char* shadow_path;

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
    char* zip_file_name = alloca(FILENAME_MAX);
    DIR* zip_dir = opendir(zip_dir_name);
    time_t latest_time = 0;
    while ((zip_file = readdir(zip_dir)) != NULL) {
        zip_file_name = zip_file->d_name;
        if (strcmp(zip_file_name, ".") == 0
                || strcmp(zip_file_name, "..") == 0)
            continue;
        // make relative path to the zip file
        char fixed_path[strlen(zip_file_name) + strlen(zip_dir_name)];
        printf("dir name: %s zip file name: %s\n",zip_dir_name,  zip_file_name);
        memset(fixed_path, 0, sizeof(fixed_path));
        sprintf(fixed_path, "%s/%s", zip_dir_name, zip_file_name);


        // open zip file in dir
        struct zip* temp_archive;
        if (!(temp_archive = zip_open(fixed_path, ZIP_RDONLY, 0))) {
            printf("ERROR OPENING ARCHIVE AT %s\n", fixed_path);
        }
        // find file in temp archive

        if (!zip_stat(temp_archive, folder_path, 0, &zipstbuf) 
                || !zip_stat(temp_archive, path + 1, 0, &zipstbuf)) {
            double dif;  
            if ((dif = difftime(zipstbuf.mtime,  latest_time)) >= 0) {
                zip_close(latest_archive);
                latest_archive = temp_archive;
                latest_time = zipstbuf.mtime;
                printf("DIF: %f\n", dif);
            }

            printf("FOUND ENTRY IN AN ARCHIVE\n");
        } else {
            printf("ENTRY NOT IN HERE\n");
            zip_close(temp_archive);
        }
    }
    if(!closedir(zip_dir))
        printf("successfully closed dir\n");
    return latest_archive;

}
/** 
 *  retrieves the attributes of a specific file / directory
 *  - returns the attributes of the LATEST file / directory
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


    // check each zip-file until u find the latest one
    struct zip* latest_archive = find_latest_archive(path);

    struct zip_stat zipstbuf;
    if (strlen(path)== 1 || !zip_stat(latest_archive, folder_path, 0, &zipstbuf)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (!zip_stat(latest_archive, path + 1, 0, &zipstbuf)) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = zipstbuf.size;


    } else {
        if (latest_archive)
            zip_close(latest_archive);
        return -ENOENT;
    }
    if (latest_archive)
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

    GArray* added_entries = g_array_new(TRUE, TRUE, sizeof(char*));
    DIR* zip_dir = opendir(zip_dir_name);
    struct dirent* zip_file;
    // find the paths to things in the given path
    while((zip_file = readdir(zip_dir)) != NULL) {
        const char* zip_file_name = zip_file->d_name;
        printf("FILE NAME %s\n", zip_file_name);
        if (strcmp(zip_file_name, ".") == 0
                || strcmp(zip_file_name, "..") ==0)
            continue;

        // make relative path to the zip file
        char fixed_path[strlen(zip_file_name) + strlen(zip_dir_name) + 2];
        memset(fixed_path, 0, strlen(fixed_path));
        memcpy(fixed_path, zip_dir_name, strlen(zip_dir_name));
        fixed_path[strlen(zip_dir_name)] = '/';
        memcpy(fixed_path + strlen(zip_dir_name) + 1, zip_file_name, strlen(zip_file_name));
        printf("FIXED PATH TO ZIP FILE: %s\n", fixed_path);
        // open zip file in dir
        struct zip* temp_archive;
        if (!(temp_archive = zip_open(fixed_path, ZIP_RDONLY, 0))) {
            printf("ERROR OPENING ARCHIVE AT %s\n", fixed_path);
        }
        int numEntries = zip_get_num_entries(temp_archive, ZIP_FL_UNCHANGED);
        for (int i = 0; i < numEntries; i++) {
            const char* zip_entry_name = zip_get_name(temp_archive, i, 0);
            printf("UNCHANGED ENTRY NAME %s\n", zip_entry_name);
            char temp[strlen(zip_entry_name)];
            strcpy(temp, zip_entry_name);
            char* fuse_name;
            // fix libzip path to match the format in fuse
            if (zip_entry_name[strlen(zip_entry_name) - 1] == '/') {
                fuse_name = alloca(strlen(zip_entry_name) + 1);
                temp[strlen(zip_entry_name) - 1] = '\0';
                strcpy(fuse_name + 1, temp);
                fuse_name[0] = '/';
            } else {
                fuse_name = alloca(strlen(zip_entry_name) + 2);
                strcpy(fuse_name + 1, temp);
                fuse_name[0] = '/';
            }
            printf("ENTRY NAME: %s\n", fuse_name);

            // check if the current file is in the directory in the given path
            char* temp_path = strdup(path);
            char* temp_fuse_path = strdup(fuse_name);
            int notInPath = strcmp(dirname(temp_fuse_path), path);
            free(temp_path);
            free(temp_fuse_path);
            char* ele;
            int inserted = 0;
            for (int j = 0; (ele = g_array_index(added_entries, char*, j)) != 0; j++) {
                if (!strcmp(ele, fuse_name)) {
                    inserted = 1;
                    break;
                }

            }
            // find file in system, add to buffer
            struct stat buffer;
            if (!inserted && !notInPath && zipfs_getattr(fuse_name, &buffer)== 0) {
                g_array_append_val(added_entries, fuse_name);
                filler(buf, basename(fuse_name), NULL, 0);
            }
        }

        zip_close(temp_archive);
    }    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);


    if (!closedir(zip_dir))
        printf("successfully closed dir\n");
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

    //find latest archive with this file
    struct zip* latest_archive = find_latest_archive(path);
    struct zip_file* file = zip_fopen(latest_archive, path + 1, 0);
    if (!latest_archive || !file) {
        printf("%s not found\n", path);
        return -1;
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
    /*
       char folder_path[strlen(path)];
       strcpy(folder_path, path + 1);
       folder_path[strlen(path) - 2] = '/';
       folder_path[strlen(path - 1)] = '\0';
       if (zip_name_locate(archive, path + 1, 0) >= 0 
       || zip_name_locate(archive, folder_path, 0)) {
       return 0;
       }
       */
    return 0;
}

/** flushes cached changes / writes to directory
 * - flushes the entire cache when called
 * @param path is the path of the file
 * @param isdatasync is not used
 * @param fi is not used
 * @return 0 on success, nonzero if cache is empty or other error occured.
 */
int zipfs_fsync(const char* path, int isdatasync, struct fuse_file_info* fi) {
    // create archive name
    char num[16] = {'\0'};
    char hex_name[33] = {'\0'};
    syscall(SYS_getrandom, num, sizeof(num), GRND_NONBLOCK);

    for (int i = 0; i < 16; i++) {
        sprintf(hex_name + i*2,"%02X", num[i]);
    }

    // create zip archive name
    char archive_path[strlen(zip_dir_name) + strlen(hex_name) + 1];
    strcat(archive_path, zip_dir_name);
    archive_path[strlen(zip_dir_name)] =  '/';
    strcat(archive_path + strlen(zip_dir_name) + 1, hex_name);

    /** zip cache
     * - using the system() call and the "real" zip command
     * - in the future it may be worthwhile to use libzip and recursively make dirs/files
     */
    char cwd[PATH_MAX];
    memset(cwd, 0, strlen(cwd));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        printf("error getting current working directory\n");
    printf("CWD: %s\n", cwd);
    char zip_dir_path[PATH_MAX + strlen(zip_dir_name) + 1];
    memset(zip_dir_path, 0, strlen(zip_dir_path));
    sprintf(zip_dir_path, "%s/%s", cwd, zip_dir_name);
    printf("DIR NAME: %s\n", zip_dir_path);

    char command[strlen(shadow_path) + strlen(zip_dir_path) + strlen(hex_name) + 50];
    memset(command, 0, strlen(command));
    sprintf(command, "cd %s; zip %s *; mv %s.zip %s; rm -rf *", 
            shadow_path, hex_name, hex_name, zip_dir_path);
    printf("MAGIC COMMAND: %s\n", command);
    system(command);
    chdir(cwd);

    return 0;
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
    printf("WRITE:%s to  %s\n", buf, path);
    (void)fi;
    // read old data from latest archive with path
    struct stat stbuf;
    zipfs_getattr(path, &stbuf);
    unsigned int file_size = stbuf.st_size;
    char new_buf[file_size + size + offset];
    memset(new_buf, 0, file_size + size + offset);
    struct zip* latest_archive = find_latest_archive(path);
    zip_file_t* file =  zip_fopen(latest_archive, path + 1, 0);
    zip_fread(file, new_buf, file_size);
    zip_fclose(file);
    // concat new data into buffer
    memcpy(new_buf + offset, buf, strlen(buf) + 1);

    printf("WRITING: %s\n", new_buf);
    // write to new file source
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    int shadow_file = open(shadow_file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR);
    if (pwrite(shadow_file, new_buf, sizeof(new_buf), 0) == -1)
        printf("error writing to shadow file\n");
    if (close(shadow_file))
        printf("error closing shadow file\n");

    /*

       zip_source_t* new_source;
       if ((new_source = zip_source_buffer(archive, new_buf, sizeof(new_buf), 0)) == NULL
       || zip_file_add(archive, path + 1, new_source, ZIP_FL_OVERWRITE) < 0) {
       zip_source_free(new_source);
       printf("error adding new file source %s\n", zip_strerror(archive));
       }

       zip_close(archive);
       archive = zip_open(zip_name, 0, 0);
       */
    zipfs_fsync(NULL, 0, 0);
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
    // create path to file
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    int shadow_file = open(shadow_file_path, O_CREAT | O_WRONLY);
    if (shadow_file == -1) {
        printf("error making shadow file descriptor\n");
        printf("ERRNO: %s\n", strerror(errno));
    }
    if (close(shadow_file)) {
        printf("error closing shadow file descriptor\n");
        printf("ERRNO: %s\n", strerror(errno));
    }
    /*
    // char mt [0];
    zip_source_t* dummy = zip_source_buffer(archive, NULL, 0, 0);
    zip_file_add(archive, path + 1, dummy, ZIP_FL_OVERWRITE);
    zip_close(archive);
    archive = zip_open(zip_name, 0, 0);
    */
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
    /*
       zip_int64_t file_index = zip_name_locate(archive, path + 1, 0);
       zip_delete(archive, file_index);
       zip_close(archive);
       archive = zip_open(zip_name, 0, 0);
       */
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
    /*
       (void)mode;
       zip_dir_add(archive, path + 1, ZIP_FL_ENC_UTF_8);
       zip_close(archive);
       archive = zip_open(zip_name, 0, 0);
       */
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
    /*
       printf("RENAME: from: %s == to: %s\n", from, to);

       zip_int64_t old_file_index = zip_name_locate(archive, from + 1, 0);
       zip_file_rename(archive, old_file_index, to + 1, 0);

       zip_close(archive);
       archive = zip_open(zip_name, 0, 0);
       */

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
    /*
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
    */

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
    free(shadow_path);
    zip_close(archive);

}
/** represents available functionality */
static struct fuse_operations zipfs_operations = {
    .getattr = zipfs_getattr,
    .readdir = zipfs_readdir,
    .read = zipfs_read,
    .fsync = zipfs_fsync,
    //.fsyncdir = zipfS_fsyncdir,
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
 * ./file-system <options> <mount point>  <dir>
 * temporary~!
 */
int
main(int argc, char *argv[]) { 
    zip_dir_name = argv[--argc];
    //zip_name = argv[--argc];
    //archive = zip_open(zip_name, 0, error);
    char* newarg[argc];
    for (int i = 0; i < argc; i++) {
        newarg[i] = argv[i];
        printf("%s\n", newarg[i]);
    }
    // construct shadow directory name
    char shadow_name[10] = {0};
    sprintf(shadow_name,"PID%d" , getpid());
    printf("shadow dir name: %s\n", shadow_name);

    // construct program directory path
    char* temp_path = alloca(PATH_MAX * sizeof(char));
    memset(temp_path, 0, strlen(temp_path));
    strcat(temp_path, "~/.cache/zipfs/");
    printf("shadow dir path: %s\n", temp_path);
    wordexp_t path;
    wordexp(temp_path, &path, 0);
    shadow_path = strdup(*(path.we_wordv));
    wordfree(&path);
    printf("expanded dir path: %s\n", shadow_path);

    // make program directory if it doesn't exist yet
    if (mkdir(shadow_path, S_IRWXU)) {
        printf("error making program directory ERRNO: %s\n", strerror(errno));
    }
    // construct shadow directory path
    strcat(shadow_path, shadow_name);
    strcat(shadow_path, "/");

    // make the directory
    if (mkdir(shadow_path, S_IRWXU)) {
        printf("error making shadow directory\n");
        printf("ERRNO: %s\n", strerror(errno));
    }


    return fuse_main(argc, newarg, &zipfs_operations, NULL);

}
