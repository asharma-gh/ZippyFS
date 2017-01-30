/**
 * Zipfs: A toy distributed file system using libfuze and libzip
 * @author Arvin Sharma
 * @version 3.0
 */
#include "fuse_ops.h"
#include <syscall.h>
#include <zip.h>
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
#include "util.h"
#include "block_cache.h"
#include "block.h"
// proportion of outdated entries in idx file for deletion
#define GC_PROP 2
using namespace std;
/**
 * TODO:
 * - clean up stuff
 * - rewrite gc
 */
BlockCache* block_cache;

/** finds the latest zip archive with the given path,
 * if it isn't deleted */
static struct zip* find_latest_archive(const char* path, char* name, int size);

/** puts the latest entry of path in the index index into buf */
static int get_latest_entry(const char* index, int in_cache, const char* path, char* buf);

/** removes unneeded zip archives and logs in a machine specific rmlog */
static int garbage_collect();

/** records the given file in path to an index file in shadow directory */
static int record_index(const char* path, int isdeleted);

/** loads the thing in archive to shadow cache **/
static int load_to_cache(const char* path);


/** the path of the mounted directory of zip files */
static char* zip_dir_name;

/** cache path */
static char* shadow_path;

/**
 * loads file in path from archive to shdw dir
 */
int
load_to_shdw(const char* path) {
    return load_to_cache(path);
}
void
clear_shdw() {
    string cmd = "rm -rf " + (string)shadow_path + "*";
    system(cmd.c_str());

}
/**
 * loads the dir specified in path to cache
 * does not overwrite files already cached.
 * - does nothing if the file is already in cache
 * @param path is the path of the dir
 * @return -1 on error, 1 if parent was loaded, 0 if file
 */
int
load_to_cache(const char* path) {
    //clear_shdw();
    // get latest archive name
    char archive_name[PATH_MAX] = {0};
    int is_dir = 0;
    struct zip* latest_archive = find_latest_archive(path, archive_name, PATH_MAX);
    char cpy[strlen(path) + 1];
    strcpy(cpy, path);
    char* path_dir = dirname(cpy);
    if (latest_archive == NULL) {
        printf("error loading thing in path to cache\n TRYING DIR NAME!!\n");
        memset(archive_name, 0, sizeof(archive_name) / sizeof(char));
        latest_archive = find_latest_archive(path_dir, archive_name, PATH_MAX);
        if (latest_archive == NULL) {
            printf("CANNOT FIND MAIN FILE OR DIRNAME!!\n");
            return -1;
        } else {
            printf("FOUND DIR NAME\n");
            is_dir = 1;
        }
    }

    if (latest_archive) {
        zip_close(latest_archive);

        // unzip to cache
        // first create path to zip file
        char cwd[PATH_MAX];
        memset(cwd, 0, sizeof(cwd) / sizeof(char));
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            printf("error getting current working directory\n");
        char path_to_archive[1 + strlen(zip_dir_name) + strlen(archive_name)];
        sprintf(path_to_archive, "%s/%s",  zip_dir_name, archive_name);

        char unzip_command[strlen(path_to_archive) + strlen(shadow_path) + (strlen(path)*2) + 20];
        if (is_dir == 0)
            sprintf(unzip_command, "unzip  -o %s %s %s/ -d %s", path_to_archive, path + 1, path + 1, shadow_path);
        else
            sprintf(unzip_command, "unzip -o %s %s %s/ -d %s", path_to_archive, path_dir + 1, path_dir + 1, shadow_path);
        printf("UNZIPPING!!!: %s\n", unzip_command);
        system(unzip_command);
        chdir(cwd);
    }
    if (is_dir)
        return 1;
    return 0;
}

void
zippyfs_init(const char* shdw, const char* zip_dir) {
    printf("starting w/ shdw %s and zip %s\n", shdw, zip_dir);
    shadow_path = strdup(shdw);
    zip_dir_name = strdup(zip_dir);
    block_cache = new BlockCache(shdw);

}
/**
 * gets the latest entry of path in the index file index
 * @param index is the path to the index file
 * @param path is the path of the entry
 * @param in_cache is whether the index file is in cache
 * @param buf is a buffer for the result.
 * If the index file is in cache, checksum is not verified.
 * @return 0 on success, -1 if error occured;
 */
static
int
get_latest_entry(const char* index, int in_cache, const char* path, char* buf) {
    // check if index path exists
    if (access(index, F_OK) != 0)
        return -1;
    // read contents
    FILE* file  = fopen(index, "r");
    // get file size
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char contents[fsize + 1];
    memset(contents, 0, sizeof(contents) / sizeof(char));
    fread(contents, fsize, 1, file);
    contents[fsize] = '\0';
    fclose(file);

    if (!in_cache) {
        if (Util::verify_checksum(contents) == -1)
            return -1;

    }

    const char delim[2] = "\n";
    char* token;
    int in_index = 0;
    char contents_cpy[strlen(contents)];
    strcpy(contents_cpy, contents);
    char* saveptr;
    token = strtok_r(contents_cpy, delim, &saveptr);
    while (token != NULL) {
        char token_path[PATH_MAX];
        sscanf(token, "%s %*u %*f %*d", token_path);
        if (strcmp(token_path, path) == 0) {
            in_index = 1;
            memset(buf, 0, sizeof(buf) / sizeof(char));
            strcpy(buf, token);
        }
        token = strtok_r(NULL, delim, &saveptr);
    }
    if (!in_index)
        return -1;
    return 0;
}

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
    printf("FINDING LATEST ARCHIVE CONTAINING: %s %s %d\n", path, name, size);
    /** Finds latest archive based on index files **/
    DIR* dir = opendir(zip_dir_name);
    if (dir == NULL) {
        printf("ERROR opening dir ERRNO: %s\n", strerror(errno));
        return NULL;
    }
    struct dirent* entry;
    char* entry_name;
    char latest_name[100];
    int is_deleted = 0;
    unsigned long long latest_time = 0;
    int exists = 0;
    while ((entry = readdir(dir)) != NULL) {
        entry_name = entry->d_name;
        if (strcmp(entry_name, ".") == 0
                || strcmp(entry_name, "..") == 0
                || strlen(entry_name) < 4
                || strcmp(entry_name + (strlen(entry_name) - 4), ".idx" ) != 0) {

            continue;
        }
        // make path to index file
        char path_to_indx[strlen(entry_name) + strlen(zip_dir_name) + 1];
        memset(path_to_indx, 0, sizeof(path_to_indx) / sizeof(char));
        sprintf(path_to_indx, "%s/%s", zip_dir_name, entry_name);
        char last_occurence[PATH_MAX + FILENAME_MAX];
        int res = get_latest_entry(path_to_indx, 0, path, last_occurence);
        if (res == -1) {
            continue;
        }

        // so we have a file's information from the index file now
        // now we need to interpret the entry
        // entry  is in the format: PATH [permissions] time-created deleted?
        unsigned long long file_time = 0;
        int deleted = 0;
        sscanf(last_occurence, "%*s %*u %llu %d", &file_time, &deleted);

        if (file_time > latest_time) {
            latest_time = file_time;
            memset(latest_name, 0, sizeof(latest_name) / sizeof(char));
            strcpy(latest_name, entry_name);
            is_deleted = deleted;
            exists = 1;
        }
    }
    closedir(dir);
    if (exists == 0)
        return NULL;
    // open zip file and return i
    struct zip* latest_archive;
    // make relative path to the zip file
    char fixed_path[strlen(latest_name) + strlen(zip_dir_name) + 1];
    memset(fixed_path, 0, sizeof(fixed_path) / sizeof(char));
    latest_name[(strlen(latest_name) - 4)] = '\0';
    sprintf(fixed_path, "%s/%s.zip", zip_dir_name, latest_name);
    if (size > 0)
        sprintf(name, "%s.zip", latest_name);
    if (is_deleted)
        return NULL;
    latest_archive = zip_open(fixed_path, ZIP_RDONLY, 0);
    return latest_archive;
}

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

/** flushes cached changes / writes to directory
 * - flushes the entire cache when calledi
 * @return 0 on success, nonzero if cache is empty or other error occured.
 */
int
flush_dir() {
    printf("Flushing \n");
    // add checksum to index file
    // read contents of index file
    char path_to_indx[PATH_MAX + strlen(shadow_path)];
    sprintf(path_to_indx, "%s/index.idx", shadow_path);
    printf("Checking %s\n", path_to_indx);
    if (access(path_to_indx, F_OK) == -1) {
        printf("no index, no writes, exiting..\n");
        garbage_collect();
        return -1;
    }
    FILE* file  = fopen(path_to_indx, "r");
    // get file size
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char* contents = (char*)alloca(fsize + 1);
    memset(contents, 0, sizeof(contents) / sizeof(char));
    fread(contents, fsize, 1, file);
    contents[fsize] = '\0';
    fclose(file);
    // generate checksum
    uint64_t checksum = Util::crc64(contents);
    // append to file
    FILE* file_ap = fopen(path_to_indx, "a");
    fprintf(file_ap, "CHECKSUM");
    fprintf(file_ap, "%" PRIu64, checksum);
    fclose(file_ap);

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

    /**
     * zip cache
     * - using the system() call and the "real" zip command
     * - in the future it may be worthwhile to use libzip and recursively make dirs/files
     * - renames the index to the zip archive name!
     */
    char cwd[PATH_MAX];
    memset(cwd, 0, sizeof(cwd) / sizeof(char));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        printf("error getting current working directory\n");

    char command[strlen(shadow_path) + (strlen(zip_dir_name)*2) + (strlen(hex_name)*4) + PATH_MAX];
    memset(command, 0, sizeof(command) / sizeof(char));
    sprintf(command, "cd %s; zip -rm %s * -x \"*.idx\"; mv %s.zip %s; mv index.idx %s.idx; mv %s.idx %s",
            shadow_path, hex_name, hex_name, zip_dir_name, hex_name, hex_name, zip_dir_name);

    printf("MAGIC COMMAND: %s\n", command);
    system(command);
    chdir(cwd);
    // garbage_collect();
    /** signal sync program **/
    // open sync pid
    wordexp_t we;
    char tild_exp[PATH_MAX];
    char path_to_sync[PATH_MAX];
    sprintf(tild_exp, "~");
    wordexp(tild_exp, &we, 0);
    sprintf(path_to_sync, "%s/.cache/zippyfs/sync.pid", *we.we_wordv);
    wordfree(&we);
    if (access(path_to_sync, F_OK) == -1)
        printf("Error finding sync pid file\n");
    // read pid
    FILE* pid_sync = fopen(path_to_sync, "r");
    char pidstr[64];
    fgets(pidstr, 64, pid_sync);
    unsigned int pid = atoi(pidstr);
    // send signal to sync code
    int res = kill(pid, SIGUSR1);
    if (res == -1)
        printf("Error signalling, ERRNO: %s\n", strerror(errno));
    fclose(pid_sync);

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

    // construct file path in cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, sizeof(shadow_file_path) / sizeof(char));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);

    DIR* zip_dir = opendir(zip_dir_name);

    if (zip_dir == NULL) {
        printf("error opening zip dir ERRNO: %s\n", strerror(errno));
        return -1;
    }

    struct dirent* entry;
    // find the paths to things in the given path
    while((entry = readdir(zip_dir)) != NULL) {

        const char* index_file_name = entry->d_name;
        if (strcmp(index_file_name, ".") == 0
                || strcmp(index_file_name, "..") == 0
                || strlen(index_file_name) < 4
                || strcmp(index_file_name + (strlen(index_file_name) - 4), ".idx") != 0) {

            continue;
        }

        // make path to index file
        char path_to_indx[strlen(index_file_name) + strlen(zip_dir_name) + 1];
        memset(path_to_indx, 0, sizeof(path_to_indx) / sizeof(char));
        sprintf(path_to_indx, "%s/%s", zip_dir_name, index_file_name);

        // read contents
        FILE* file  = fopen(path_to_indx, "r");
        // get file size
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        rewind(file);
        char contents[fsize + 1];
        memset(contents, 0, sizeof(contents) / sizeof(char));
        fread(contents, fsize, 1, file);
        contents[fsize] = '\0';
        fclose(file);

        if (Util::verify_checksum(contents) == -1)
            continue;

        // now we need to find all of the entries which are in the given path
        // and update our hash table
        const char delim[2] = "\n";
        char* token;
        char contents_cpy[strlen(contents)];
        strcpy(contents_cpy, contents);
        char* saveptr;
        token = strtok_r(contents_cpy, delim, &saveptr);
        while (token != NULL) {
            // get path out of token
            char token_path[PATH_MAX];
            unsigned long long token_time;
            int deleted;
            sscanf(token, "%s %*u %llu %d", token_path, &token_time, &deleted);
            char* temp = strdup(token_path);
            // find out of this entry is in the directory
            int in_path = strcmp(dirname(temp), path);
            free(temp);
            if (in_path == 0) {
                cout << "token " << token << endl;
                BlockCache::index_entry val;
                string old_name;
                if (added_names.find(token_path) != added_names.end()) {
                    auto entry = added_names.find(token_path);
                    val = entry->second;

                    // entry is in the hash table, compare times
                    if (val.added_time <= token_time) {
                        // this token is a later version. Create new hash-table entry
                        BlockCache::index_entry new_entry;
                        new_entry.added_time = token_time;
                        new_entry.deleted = deleted;
                        // add to hash table

                        added_names[token_path] = new_entry;
                    }
                } else {
                    // it is not in the hash table so we need to add it
                    BlockCache::index_entry new_entry;
                    new_entry.added_time = token_time;
                    new_entry.deleted = deleted;

                    added_names[token_path] = new_entry;
                }
            }

            token = strtok_r(NULL, delim, &saveptr);
        }
    }
    // check file cache
    string index_path = (string)shadow_path + "/index.idx";
    ifstream in_file(index_path);
    string curline;
    while (getline(in_file, curline)) {
        char token_path[PATH_MAX];
        unsigned long long token_time;
        int deleted;
        sscanf(curline.c_str(), "%s %*u %llu %d", token_path, &token_time, &deleted);
        //  cout << "token path " << token_path << endl;
        char* temp = strdup(token_path);
        // find out of this entry is in the directory
        int in_path = strcmp(dirname(temp), path);
        // cout << "in dir? " << in_path << endl;
        free(temp);
        if (in_path == 0) {
            cout << "token" << curline << endl;
            BlockCache::index_entry val;
            string old_name;
            if (added_names.find(token_path) != added_names.end()) {
                auto entry = added_names.find(token_path);
                val = entry->second;

                // entry is in the hash table, compare times
                if (val.added_time <= token_time) {
                    // this token is a later version. Create new hash-table entry
                    BlockCache::index_entry new_entry;
                    new_entry.added_time = token_time;
                    new_entry.deleted = deleted;
                    // add to hash table

                    added_names[token_path] = new_entry;
                }
            } else {
                // it is not in the hash table so we need to add it
                BlockCache::index_entry new_entry;
                new_entry.added_time = token_time;
                new_entry.deleted = deleted;

                added_names[token_path] = new_entry;
            }
        }

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

    if (!closedir(zip_dir))
        printf("successfully closed dir\n");

    return 0;
}







/**
 * removes fully updated zip archives
 * log is located in
 * @return 0 on success, non-zero otherwise
 */
static
int
garbage_collect() {
    // TODO: move files from archive with high% outdated ones
    /****
     * HashTables for efficient garbage collection
     * (Path, (time, indx file))
     * - contains latest time for each file and the index file it came from
     ****/
    typedef struct {
        unsigned long long f_time;
        string indx_file;
    } ent_info;
    map<string, ent_info> gc_table;
    /*****
     * (index path, valid entries
     * contains the number of updated entries for the index file
     * if this value is 0, the files are garbage collected
     *****/
    map<string, unsigned long long> valid_ents;


    /*****
     * (index path, num entries)
     */
    map<string, unsigned long long> num_ents;
    /****
     * (index path, list of valid entries)
     */
    map<string, unordered_set<string>> invalid_files;

    printf("GARBAGE COLLECTING . . .\n");
    // make path to rmlog
    char rmlog_path[PATH_MAX];
    sprintf(rmlog_path, "~/.config/zippyfs/");
    // tilde expansion
    wordexp_t path;
    wordexp(rmlog_path, &path, 0);
    memset(rmlog_path, 0, sizeof(rmlog_path) / sizeof(char));
    strcpy(rmlog_path, *path.we_wordv);
    wordfree(&path);

    // open directory containing rm log
    DIR* log_dir = opendir(rmlog_path);
    struct dirent* entry;
    char log_name[FILENAME_MAX];
    while((entry = readdir(log_dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
                || strcmp(entry->d_name, "..") == 0)
            continue;
        else {
            // only one entry in archive
            strcpy(log_name, entry->d_name);
            break;
        }
    }
    closedir(log_dir);
    // make machine specific log file in zip directory if it doesn't exist
    char* path_local_log = (char*)malloc(PATH_MAX + strlen(log_name));
    memset(path_local_log, 0, sizeof(path_local_log) / sizeof(char));
    sprintf(path_local_log, "%s/rmlog/%s", zip_dir_name, log_name);
    int log_fd = open(path_local_log, O_CREAT | O_APPEND, S_IRWXU);
    if (log_fd == -1)
        printf("Error making zip dir rm log ERRNO: %s\n", strerror(errno));
    close(log_fd);

    // scan each archive to see if is out dated
    // create path to zip dir
    DIR* zip_dir = opendir(zip_dir_name);
    struct dirent* archive_entry;
    while ((archive_entry = readdir(zip_dir)) != NULL) {
        if (strcmp(archive_entry->d_name, ".") == 0
                || strcmp(archive_entry->d_name, "..") == 0
                || strlen(archive_entry->d_name) < 4
                || strcmp(archive_entry->d_name + (strlen(archive_entry->d_name) - 4), ".idx") != 0)
            continue;

        // make path to index file
        char path_to_indx[strlen(archive_entry->d_name) + strlen(zip_dir_name) + 1];
        memset(path_to_indx, 0, sizeof(path_to_indx) / sizeof(char));
        sprintf(path_to_indx, "%s%s", zip_dir_name, archive_entry->d_name);
        cout << "CHECKING " << path_to_indx << endl;
        // read contents
        FILE* file  = fopen(path_to_indx, "r");
        // get file size
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        rewind(file);
        char contents[fsize + 1];
        fread(contents, fsize, 1, file);
        contents[fsize] = '\0';
        fclose(file);
        if (Util::verify_checksum(contents) == -1)
            continue;
        const char delim[2] = "\n";
        char* token;
        char contents_cpy[strlen(contents)];
        char* save_ptr;
        strcpy(contents_cpy, contents);

        token = strtok_r(contents_cpy, delim, &save_ptr);
        valid_ents[path_to_indx] = 0;
        while (token != NULL) {
            // fetch path from token
            char token_path[PATH_MAX];
            if (strstr(token, "CHECKSUM"))
                // basically done with the file at this point
                break;
            unsigned long long file_time;
            sscanf(token, "%s %*u %llu %*d", token_path, &file_time);
            // if this is actually the latest version, the file is not outdated we can move on
            num_ents[path_to_indx]++;
            if (gc_table.find(token_path) != gc_table.end()) {
                // entry is in the hash table, compare times
                if (gc_table.find(token_path)->second.f_time <= file_time) {
                    // we have an updated entry
                    // decrease valid ents for an indx file
                    valid_ents[gc_table[token_path].indx_file]--;
                    invalid_files[gc_table[token_path].indx_file].insert(token_path);
                    cout << "INV FILE CONTENTS -------------" << endl;
                    for (auto f : invalid_files) {
                        cout << "CHECKING " << f.first << endl;
                        for (auto e : f.second) {
                            cout << e << endl;
                        }
                    }
                    // add to hash table
                    ent_info ent;
                    ent.indx_file = path_to_indx;
                    ent.f_time = file_time;
                    gc_table[token_path] = ent;
                    valid_ents[path_to_indx]++;
                }
            } else {
                // make entry for this item
                valid_ents[path_to_indx]++;
                ent_info ent;
                ent.indx_file = path_to_indx;
                ent.f_time = file_time;
                gc_table[token_path] = ent;

            }
            token = strtok_r(NULL, delim, &save_ptr);
        }
    }

    for (auto ents : valid_ents) {
        cout << "NAME " << ents.first << endl;
        double prop = (double)ents.second / num_ents[ents.first];
        cout << "PROP " << prop << endl;
        // if (ents.second < 1)
        //  continue;
        //if (prop < GC_PROP) {

        if (invalid_files.find(ents.first) != invalid_files.end()) {
            // make path to zip archive
            string zip_path = ents.first.substr(0, ents.first.length() - 4);
            zip_path = zip_path + ".zip";
            cout << "PATH TO ARCHIVE FROM IDX " << zip_path << endl;
            struct zip* archive = zip_open(zip_path.c_str(), 0, 0);
            vector<int64_t> invalid_zip_idx;

            for (auto inv_name : invalid_files[ents.first]) {
                cout << "IDX FILE " << inv_name << endl;
                cout << "CHECKING " << inv_name << endl;
                int64_t fidx = zip_name_locate(archive, inv_name.c_str() + 1, 0);
                if (fidx == -1) {
                    cout<< "ERROR FINDING FILE IN ZIP ARCHIVE" << endl;
                    continue;
                }
                invalid_zip_idx.push_back(fidx);
            }

            for (auto idx : invalid_zip_idx) {
                cout << "DELETING AT " << idx << endl;
                zip_delete(archive, idx);
            }
            // TODO: Rewrite indx file here
            zip_close(archive);
        }
    }

    // if (prop == 1)
    return 0;
    /*
            cout << "GARBAGE COLLECTING " << ents.first << endl;
            // trim path name to just base name
            char* b_name = (char*)alloca(FILENAME_MAX * sizeof(char));
            strcpy(b_name, ents.first.c_str());
            b_name = basename(b_name);
            // write to machine log file
            FILE* log_file = fopen(path_local_log, "a");
            int res =  fprintf(log_file, "%s\n", b_name);
            if (res == -1) {
                printf("ERROR WRITING, ERRNO? %s\n", strerror(errno));
            }
            fclose(log_file);
            **
             * now locally delete zip file and index, since its
             * outdated!


            b_name[strlen(b_name) - 4] = '\0';
            // create command to remove both index and zip file
            char command[(strlen(b_name) * 2) + (strlen(zip_dir_name) * 2) + 10];
            sprintf(command, "rm %s/%s.zip; rm %s/%s.idx", zip_dir_name, b_name, zip_dir_name, b_name);
            system(command);

        }

        free(path_local_log);
        closedir(zip_dir);
        return 0;
        */
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
    block_cache->load_from_shdw(path);
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
     * will eventually have the name <archive name>.idx upon flushing
     */
    // create path to index file
    char idx_path[strlen(shadow_path) + 15];
    sprintf(idx_path, "%s/index.idx", shadow_path);

    // open index file
    int idxfd = open(idx_path, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
    /**
     * format for index file entry
     * PATH [PERMISSIONS] MODIFY-TIME DELETED?\n
     * e.g.:
     * /foo/bar [RWX] 1024 0\n
     * /foo/bar [R] 10 1\n
     * NEW CHANGE:
     * permissions now rep. as mode bits!!
     */
    // make path to file in cache
    char path_to_file[PATH_MAX + strlen(shadow_path) + 2];
    memset(path_to_file, 0, sizeof(path_to_file) / sizeof(char));
    sprintf(path_to_file, "%s%s", shadow_path, path+1);
    char input[PATH_MAX + 1024];
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));
    if (stat(path_to_file, &buf) == -1)
        printf("error retrieving file information for %s\n ERRNO: %s\n",
               path, strerror(errno));

    unsigned long long act_time = Util::get_time();
    sprintf(input, "%s %u %llu %d\n", path, buf.st_mode, act_time, deleted);
    printf("====WRITING THE FOLLOWING TO INDEX====\n%s\n", input);
    if (write(idxfd, input, strlen(input)) == -1) {
        printf("Error writing to idx file\n");
    }
    close(idxfd);

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
int
zippyfs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    printf("WRITE to  %s\n",  path);
    (void)fi;
    block_cache->write(path, (uint8_t*)buf, size, offset);
    if (block_cache->flush_to_shdw(0) == 0) {
        cout << "ok" << endl;
    }
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
    if (block_cache->flush_to_shdw(0) == 0) {

    }
    // flush_dir();
    return 0;
}

/**
 * check user permissions of a file
 * @param path is the path to the file
 * @param mask is for permissions, unus ed
 * @return 0 for success, non-zero otherwise
 */
int
zippyfs_access(const char* path, int mode) {
    printf("ACCESS: %s %d\n", path, mode);
    (void)mode;
    if (block_cache->in_cache(path) == 0)
        return 0;
    if (strcmp(path, "/") == 0)
        return 0;

    load_to_cache(path);
    // add new file to cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, sizeof(shadow_file_path) / sizeof(char));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);

    return access(shadow_file_path, mode);
}

/**
 * changes permissions of thing in path
 */
int
zippyfs_chmod(const char* path, mode_t  mode) {
    load_to_cache(path);
    // add new file to cache
    char shadow_file_path[strlen(path) + strlen(shadow_path)];
    memset(shadow_file_path, 0, sizeof(shadow_file_path) / sizeof(char));
    strcat(shadow_file_path, shadow_path);
    strcat(shadow_file_path, path+1);

    int res = chmod(shadow_file_path, mode);
    record_index(path, 0);
    return res;
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
    flush_dir();
    // delete process cache directory
    char removal_cmd[PATH_MAX + 12];
    sprintf(removal_cmd, "rm -rf %s", shadow_path);
    system(removal_cmd);
    rmdir(shadow_path);
    free(shadow_path);
    free(zip_dir_name);
}
