#include "fuse_ops.h"

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
static struct fuse_operations zippyfs_operations = {
    .getattr = zippyfs_getattr,
    .read = zippyfs_read,
    .mknod = zippyfs_mknod,
    .unlink = zippyfs_unlink,
    .mkdir = zippyfs_mkdir,
    .rename = zippyfs_rename,
    .write = zippyfs_write,
    .truncate = zippyfs_truncate,
    .access = zippyfs_access,
    .open = zippyfs_open,
    .rmdir = zippyfs_rmdir,
    .chmod = zippyfs_chmod,
    .utimens = zippyfs_utimens,
    .destroy = zippyfs_destroy,
};

/**
 * The main method of this program
 * calls fuse_main to initialize the filesystem
 * ./file-system <options> <mount point>  <ABSOLUTE PATH TO dir>
 */
int
main(int argc, char *argv[]) {
    const char* zip_dir = argv[--argc];
    char* newarg[argc];
    for (int i = 0; i < argc; i++) {
        newarg[i] = argv[i];
        printf("%s\n", newarg[i]);
    }

    // construct shadow directory name
    char shadow_name[10] = {0};
    sprintf(shadow_name,"PID%d", getpid());
    printf("shadow dir name: %s\n", shadow_name);

    // construct program directory path
    char temp_path[PATH_MAX];
    memset(temp_path, 0, sizeof(temp_path) / sizeof(char));
    strcat(temp_path, "~/.cache/zipfs/");
    printf("shadow dir path: %s\n", temp_path);
    wordexp_t path;
    wordexp(temp_path, &path, 0);
    shadow_path = strdup(*(path.we_wordv));
    wordfree(&path);
    printf("expanded dir path: %s\n", shadow_path);
    char shadow_path[PATH_MAX];

    // make program directory if it doesn't exist yet
    if (mkdir(shadow_path, S_IRWXU)) {
        printf("error making zipfs directory ERRNO: %s\n", strerror(errno));
    }
    // construct shadow directory path
    strcat(shadow_path, shadow_name);
    strcat(shadow_path, "/");

    // init ops
    zippyfs_init(zip_dir, shadow_path);
    // make the directory
    if (mkdir(shadow_path, S_IRWXU)) {
        printf("error making shadow directory\n");
        printf("ERRNO: %s\n", strerror(errno));
    }
    // construct process-local rmlog path and dir
    char rmlog_path[strlen(zip_dir_name) + 10];
    memset(rmlog_path, 0, sizeof(rmlog_path) / sizeof(char));
    sprintf(rmlog_path, "%s/rmlog", zip_dir_name);
    if (mkdir(rmlog_path, S_IRWXU))
        printf("error making rmlog dir ERRNO:%s\n", strerror(errno));

    // construct machine-local rmlog path and dir
    char machine_rmlog_path[strlen(zip_dir_name) + PATH_MAX];
    memset(machine_rmlog_path, 0, sizeof(machine_rmlog_path) / sizeof(char));
    char tild_exp[PATH_MAX];
    sprintf(tild_exp, "~");
    wordexp(tild_exp, &path, 0);

    sprintf(machine_rmlog_path, "%s/.config/zipfs/", *path.we_wordv);

    printf("machine path %s\n", machine_rmlog_path);
    int made_dir = 1;
    if (mkdir(machine_rmlog_path, S_IRWXU)) {
        made_dir = 0;
        printf("error making machine rmlog path ERRNO: %s\n", strerror(errno));
    }

    // only do this once per machine
    if (made_dir) {
        // create archive name
        char num[16] = {'\0'};
        char hex_name[33] = {'\0'};
        syscall(SYS_getrandom, num, sizeof(num), GRND_NONBLOCK);

        for (int i = 0; i < 16; i++) {
            sprintf(hex_name + i*2,"%02X", num[i]);
        }
        // make log file
        char log_name[strlen(hex_name) + 1];
        memset(log_name, 0, sizeof(log_name) / sizeof(char));
        sprintf(log_name, "%s", hex_name);
        printf("NAME OF FILE %s\n", log_name);
        // make path to log file
        char log_path[strlen(log_name) + PATH_MAX];
        memset(log_path, 0, sizeof(log_path) / sizeof(char));
        sprintf(log_path, "%s/.config/zipfs/%s", *path.we_wordv, log_name);
        // create file
        printf("file path: %s", log_path);
        int fd = open(log_path, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IXUSR);
        close(fd);

    }
    wordfree(&path);
    return fuse_main(argc, newarg, &zipfs_operations, NULL);
}
