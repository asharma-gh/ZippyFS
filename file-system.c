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
#include <stdint.h>
#include <inttypes.h>
#include <linux/random.h>
/** using glib over switching languages */
#include <glib.h>

/** the name of the mounted directory of zip files */
static char* zip_dir_name;

/** cache for writes */
static char* shadow_path;

/**
 * retrieve the latest archive with the given path
 * @param path is the path of the entry to find
 * @param name is the name of the archive, NULL if unneeded
 * @param size is the size of name
 * @return the pointer to the zip_file, NULL if it doesnt exist
 */
static
struct zip* 
find_latest_archive(const char* path, char* name, int size) {
    printf("FINDING LATEST ARCHIVE CONTAINING: %s\n", path);
    int len = strlen(path);
    char folder_path[len + 1];
    if (strlen(path) > 1) {
        strcpy(folder_path, path + 1);
        folder_path[len - 1] = '/';
        folder_path[len] = '\0'; 
    }
    struct zip_stat zipstbuf;

    /** Finds latest archive based on index files **/
    DIR* dir = opendir(zip_dir_name);
    struct dirent* entry; 
    char* entry_name = alloca(FILENAME_MAX);
    char* latest_name = alloca(FILENAME_MAX);
    time_t latest_t = 0;
    while ((entry = readdir(dir)) != NULL) {
        entry_name = entry->d_name;
        printf("ENTRY NAME  %s\n", entry_name);
        if (strcmp(entry_name, ".") == 0
                || strcmp(entry_name, "..") == 0
                || strlen(entry_name) < 4
                || strcmp(entry_name + (strlen(entry_name) - 4), ".idx" ) != 0) {
            printf("not valid index file\n");
            continue;
        }

        // make path to index file
        char path_to_indx[strlen(entry_name) + strlen(zip_dir_name) + 1];
        memset(path_to_indx, 0, strlen(path_to_indx) * sizeof(char));
        sprintf(path_to_indx, "%s/%s", zip_dir_name, entry_name);

        // read contents
        FILE* file  = fopen(path_to_indx, "r");
        // get file size
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        rewind(file);
        char* contents = alloca(fsize + 1);
        char* checksum;
        memset(contents, 0, strlen(contents) * sizeof(char));
        memset(checksum, 0, strlen(checksum) * sizeof(char));
        fread(contents, fsize, 1, file);
        contents[fsize] = '\0';
        fclose(file);
        printf("---Contents---\n%s\n", contents);
        if ((checksum = strstr(contents, "CHECKSUM")) == NULL) {
            printf("malformed index file\n");
            continue;
        }
        printf("---Checksum---\n%s\n", checksum);
        char* checksum_cpy = strdup(checksum);
        checksum[0] = '\0';
        printf("--- New Contents ---\n%s\n", contents);
        // extract numeric value of checksum
        // checksum_cpy + 8 = numeric value
        int64_t checksum_val;
        char* endptr;
        checksum_val = strtoull(checksum_cpy + 8, &endptr, 10);
        printf("-----Value for checksum after conversion\n");
        printf("%"PRIu64, checksum_val);
        fflush(stdout);

        free(checksum_cpy);

        
                
      //  uint64_t checksum = crc64(contents);

        // verify checksum
        // ok we have a valid index file
        // find our file entry in it
        // check if file is deleted
        // check times
        // update latest file and time
    }



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
        char fixed_path[strlen(zip_file_name) + strlen(zip_dir_name) + 1];
        //      printf("dir name: %s zip file name: %s\n",zip_dir_name,  zip_file_name);
        memset(fixed_path, 0, sizeof(fixed_path));
        sprintf(fixed_path, "%s/%s", zip_dir_name, zip_file_name);


        // open zip file in dir
        struct zip* temp_archive;
        int err;
        if (!(temp_archive = zip_open(fixed_path, ZIP_RDONLY, &err))) {
            printf("ERROR OPENING ARCHIVE AT %s\n", fixed_path);
            printf("ERROR: %d\n", err);
        }
        // find file in temp archive
        if (!zip_stat(temp_archive, folder_path, 0, &zipstbuf) 
                || !zip_stat(temp_archive, path + 1, 0, &zipstbuf)) {

            double dif;
            // check if its newer
            if ((dif = difftime(zipstbuf.mtime,  latest_time)) >= 0) {
                zip_close(latest_archive);
                latest_archive = temp_archive;
                latest_time = zipstbuf.mtime;
                if (name != NULL) {
                    memset(name, 0, size * sizeof(char));
                    strcpy(name, zip_file_name);
                }
                //            printf("DIF: %f\n", dif);
            }

            //       printf("FOUND ENTRY IN AN ARCHIVE\n");
        } else {
            //   printf("ENTRY NOT IN HERE\n");
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
    // NEW!
    // checks the cache first
    // construct file path in cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    if (stat(shadow_file_path, stbuf) == -1) {
        printf("Item not in cache.. checking main dir\n");
        memset(stbuf, 0, sizeof(struct stat));
    } else {
        printf("Found item in cache\n");
        return 0; 
    }

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
    struct zip* latest_archive = find_latest_archive(path, NULL, 0);

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

    // NEW!:
    // checks cache first
    // construct file path in cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    GArray* added_entries = g_array_new(TRUE, TRUE, sizeof(char*));

    DIR* dp = opendir(shadow_file_path);
    if (dp == NULL) {
        printf("Not in cache, checking main dir..\n");
    } else {
        printf("Found item in cache\n");
        struct dirent* de;
        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name,".") == 0
                    || strcmp(de->d_name, "..") ==0)
                continue;

            printf("de name:|%s|\n", de->d_name);
            if (filler(buf, de->d_name, NULL, 0))
                break;
            char* entry = strdup(de->d_name);
            g_array_append_val(added_entries, entry);

        }

        closedir(dp);

    }

    DIR* zip_dir = opendir(zip_dir_name);
    struct dirent* zip_file;
    // find the paths to things in the given path
    while((zip_file = readdir(zip_dir)) != NULL) {
        const char* zip_file_name = zip_file->d_name;
        printf("FILE NAME |%s|\n", zip_file_name);
        if (strcmp(zip_file_name, ".") == 0
                || strcmp(zip_file_name, "..") ==0)
            continue;

        // make relative path to the zip file
        char fixed_path[strlen(zip_file_name) + strlen(zip_dir_name) + 1];
        memset(fixed_path, 0, sizeof(fixed_path));
        sprintf(fixed_path, "%s/%s", zip_dir_name, zip_file_name);
        // printf("FIXED PATH TO ZIP FILE: %s\n", fixed_path);
        // open zip file in dir
        struct zip* temp_archive;
        if (!(temp_archive = zip_open(fixed_path, ZIP_RDONLY, 0))) {
            printf("ERROR OPENING ARCHIVE AT %s\n", fixed_path);
        }
        int numEntries = zip_get_num_entries(temp_archive, ZIP_FL_UNCHANGED);
        for (int i = 0; i < numEntries; i++) {
            const char* zip_entry_name = zip_get_name(temp_archive, i, 0);
            //    printf("UNCHANGED ENTRY NAME %s\n", zip_entry_name);
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
            printf("ENTRY NAME: |%s|\n", fuse_name + 1);

            // check if the current file is in the directory in the given path
            char* temp_path = strdup(path);
            char* temp_fuse_path = strdup(fuse_name);
            int notInPath = strcmp(dirname(temp_fuse_path), path);
            free(temp_path);
            free(temp_fuse_path);
            char* ele;
            int inserted = 0;
            for (int j = 0; (ele = g_array_index(added_entries, char*, j)) != 0; j++) {
                if (!strcmp(ele, fuse_name + 1)) {
                    inserted = 1;
                    printf("PUT IN ALREADY\n");
                    break;
                }

            }
            // find file in system, add to buffer
            struct stat buffer;
            if (!inserted && !notInPath && zipfs_getattr(fuse_name, &buffer)== 0) {
                char* entry = strdup(fuse_name + 1);
                g_array_append_val(added_entries, entry);
                filler(buf, basename(fuse_name), NULL, 0);
            }
        }

        zip_close(temp_archive);
    }
    // clean up
    char* release;
    for (int i = 0; (release = g_array_index(added_entries, char*, i))
            != 0; i++) {
        free(release);
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);


    if (!closedir(zip_dir))
        printf("successfully closed dir\n");
    return 0;

}

/**
 * lazy implementation of crc64 without a table
 * - iterates thru each bit and applies a mask
 * @param msg is the contents of the index file
 * @return the encoded message
 */
uint64_t 
crc64(const char* message) {
    uint64_t crc = 0xFFFFFFFFFFFFFFFF;
    uint64_t special_bits = 0xDEADBEEFABCD1234;
    uint64_t mask = 0;

    for(int i = 0; message[i]; i++) {
        crc ^= (uint8_t)message[i];
        for (int j = 0; j < 7; j++) {
            // I found that if I don't change the mask in respect to the last bit in crc
            // there are inputs such as "abc123" and "123abc" which will have the same hash
            mask = -(crc&1) ^ special_bits; // mask = 0xbits0 or 0xbits1 depending on crc
            crc = crc ^ mask;
            crc = crc >> 1;
        }
    }
    return crc;
}


/** flushes cached changes / writes to directory
 * - flushes the entire cache when called
 * @param path is the path of the file
 * @param isdatasync is not used
 * @param fi is not used
 * @return 0 on success, nonzero if cache is empty or other error occured.
 */
static
int
zipfs_fsync(const char* path, int isdatasync, struct fuse_file_info* fi) {

    // add checksum to index file
    // read contents of index file
    char path_to_indx[PATH_MAX + strlen(shadow_path)];
    sprintf(path_to_indx, "%s/index.idx", shadow_path);
    if (access(path_to_indx, F_OK) == -1) {
        printf("no index, no writes, exiting..\n");
        return -1;
    }
    FILE* file  = fopen(path_to_indx, "r");
    // get file size
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char* contents = alloca(fsize + 1);
    memset(contents, 0, strlen(contents) * sizeof(char));
    fread(contents, fsize, 1, file);
    contents[fsize] = '\0';
    fclose(file);
    // generate checksum
    uint64_t checksum = crc64(contents);
    // append to file
    FILE* file_ap = fopen(path_to_indx, "a");
    fprintf(file_ap, "CHECKSUM");
    fprintf(file_ap, "%"PRIu64, checksum);
    fclose(file_ap);
    printf("CHECKSUM");
    printf("%"PRIu64"\n", checksum);

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
     * - renames the index to the zip archive name!
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

    char command[strlen(shadow_path) + (strlen(zip_dir_path)*2) + (strlen(hex_name)*4) + PATH_MAX];
    memset(command, 0, strlen(command));
    sprintf(command, "cd %s; zip %s *; mv %s.zip %s; mv index.idx %s.idx; mv %s.idx %s", 
            shadow_path, hex_name, hex_name, zip_dir_path, hex_name, hex_name, zip_dir_path);
    printf("MAGIC COMMAND: %s\n", command);
    system(command);
    chdir(cwd);

    return 0;
}
/**
 * like fsync but for directories
 * @param path is the path of a file
 * @param d is unused
 * @param fi is unused
 * @return 0 on success, non-zero otherwise
 */
static
int
zipfs_fsyncdir(const char* path, int d, struct fuse_file_info* fi) {
    (void)d;
    (void)fi;
    zipfs_fsync(NULL, 0, 0);
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
    zipfs_fsync(NULL, 0, 0);
    //unused
    (void) fi;
    (void) offset;
    printf("READ: %s\n", path);
    // NEW!:
    // CHECK THE CACHE FIRST

    // construct file path in cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    int fd = open(shadow_file_path, O_RDONLY);
    if (fd == -1) {
        printf("File is not in cache\n");
    } else {
        printf("FOUND file in cache\n");
        int res = pread(fd, buf, size, offset);
        if (res == -1) {
            printf("ERROR reading file in cache\n");
        }
        printf("read %s\n", buf);
        close(fd);
        return res;
    }

    // file not in cache, checking in main dir
    // find latest archive with this file
    struct zip* latest_archive = find_latest_archive(path, NULL, 0);
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
    zip_close(latest_archive);
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
/**
 * creates and records the file in PATH in an index file
 * in cache
 * @param path is the path of a file in cache
 * @param deleted is whether this file should be considered deleted
 * @return 0 on success, non-zero otherwise
 */
static
int
record_index(const char* path, int deleted) {
    printf("NOW MAKING INDEX FILE FOR %s\n", path);
    /**
     * each write will now create/modify an index file
     * this will contain information about the newly created file
     * upon flushing, this will not be in the zip archive. It
     * will eventually have the name <archive name>.idx upon zipping
     * in fsync
     */
    // create path to index file
    char idx_path[strlen(shadow_path) + 15];
    sprintf(idx_path, "%s/index.idx", shadow_path);

    // open index file
    int idxfd = open(idx_path, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
    /** format for index file entry
     * PATH [PERMISSIONS] MODIFY-TIME DELETED?\n
     * e.g.: 
     * /foo/bar [RWX] 1024 0\n
     * /foo/bar [R] 10 1\n
     */

    // make path to file in cache
    char path_to_file[PATH_MAX + strlen(shadow_path) + 2];
    memset(path_to_file, 0, strlen(path_to_file) * sizeof(char));
    sprintf(path_to_file, "%s%s", shadow_path, path+1);
    printf("Path to file %s\n", path_to_file);
    char input[PATH_MAX + 1024];

    char permissions[6] = {0};
    strcat(permissions, "[");
    if (access(path_to_file, R_OK) == 0)
        strcat(permissions, "R");
    if (access(path_to_file, W_OK) == 0)
        strcat(permissions, "W");
    if (access(path_to_file, X_OK) == 0)
        strcat(permissions, "X");

    strcat(permissions, "]");
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));
    if (stat(path_to_file, &buf) == -1)
        printf("error retrieving file information for %s\n ERRNO: %s\n", 
                path_to_file, strerror(errno));

    time_t file_time = buf.st_mtime;
    double act_time = difftime(file_time, 0);

    sprintf(input, "%s %s %f %d\n", path, permissions, act_time, deleted);
    if (write(idxfd, input, strlen(input)) != strlen(input)) {
        printf("Error writing to idx file\n");
    }

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
    // get latest archive name
    char archive_name[PATH_MAX] = {0};
    struct zip* latest_archive = find_latest_archive(path, archive_name, PATH_MAX);
    if (latest_archive == NULL) {
        printf("error writing\n");
    }
    zip_close(latest_archive);

    // unzip to cache
    // first create path to zip file
    char cwd[PATH_MAX];
    memset(cwd, 0, strlen(cwd));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        printf("error getting current working directory\n");
    printf("CWD: %s\n", cwd);
    char path_to_archive[strlen(cwd) + 2 + strlen(zip_dir_name) + strlen(archive_name)];

    sprintf(path_to_archive, "%s/%s/%s", cwd, zip_dir_name, archive_name);
    char unzip_command[strlen(path_to_archive) + strlen(shadow_path) + 20];
    sprintf(unzip_command, "unzip -n %s -d %s", path_to_archive, shadow_path);
    printf("UNZIPPING!!!: %s\n", unzip_command);
    system(unzip_command);
    chdir(cwd);

    // write to new file source
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    int shadow_file = open(shadow_file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR);
    if (pwrite(shadow_file, buf, size, offset) == -1)
        printf("error writing to shadow file\n");
    if (close(shadow_file))
        printf("error closing shadow file\n"); 

    // record to index file
    record_index(path, 0);


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
    int shadow_file = open(shadow_file_path, O_CREAT | O_WRONLY, S_IRWXU);
    if (shadow_file == -1) {
        printf("error making shadow file descriptor\n");
        printf("ERRNO: %s\n", strerror(errno));
    }
    if (close(shadow_file)) {
        printf("error closing shadow file descriptor\n");
        printf("ERRNO: %s\n", strerror(errno));
    }
    // record to index file
    record_index(path, 0);

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
    record_index(path, 1);
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

    // create path to file
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    int shadow_file = mkdir(shadow_file_path, mode);
    if (shadow_file == -1) {
        printf("error making shadow file descriptor\n");
        printf("ERRNO: %s\n", strerror(errno));
    }
    // record to index
    record_index(path, 0);
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
    printf("TRUNCATE: %s\n", path);   

    // get latest archive name
    char archive_name[PATH_MAX] = {0};
    struct zip* latest_archive = find_latest_archive(path, archive_name, PATH_MAX);
    if (latest_archive == NULL) {
        printf("error writing\n");
    }
    zip_close(latest_archive);
    // unzip to cache
    // first create path to zip file
    char cwd[PATH_MAX];
    memset(cwd, 0, strlen(cwd));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        printf("error getting current working directory\n");
    printf("CWD: %s\n", cwd);    
    char path_to_archive[strlen(cwd) + 2 + strlen(zip_dir_name) + strlen(archive_name)];

    sprintf(path_to_archive, "%s/%s/%s", cwd, zip_dir_name, archive_name);
    char unzip_command[strlen(path_to_archive) + strlen(shadow_path) + 20];
    sprintf(unzip_command, "unzip -n %s -d %s", path_to_archive, shadow_path);

    printf("UNZIPPING!!!: %s\n", unzip_command);
    system(unzip_command);
    chdir(cwd);

    // add new file to cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, strlen(shadow_file_path));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);
    int shadow_file = open(shadow_file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR);
    if (ftruncate(shadow_file, size)  == -1)
        printf("error writing to shadow file\n");
    if (close(shadow_file))
        printf("error closing shadow file\n");

    // record to index
    record_index(path, 0);

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

}
/** represents available functionality */
static struct fuse_operations zipfs_operations = {
    .getattr = zipfs_getattr,
    .readdir = zipfs_readdir,
    .read = zipfs_read,
    .fsync = zipfs_fsync,
    .fsyncdir = zipfs_fsyncdir,
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
        printf("error making zipfs directory ERRNO: %s\n", strerror(errno));
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
