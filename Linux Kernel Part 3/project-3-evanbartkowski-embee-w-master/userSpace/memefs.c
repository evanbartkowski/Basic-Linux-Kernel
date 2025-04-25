/********************************************************************************
* File: memefs.c                                                                *
* Course: CMSC421 Operating Systems, Fall 2024, UMBC                            *
*                                                                               *
* About: Program main; Sets up mountpoint for filesystem driver.                *
*                                                                               *
********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define FUSE_USE_VERSION 31
#include <fuse.h>
#include "img_funcs.h"
#include "driver_funcs.h"
/////////////////////////////////////////////////////////////////////////////
// Filesystem options structure
static struct options {
    const char *volume_label;
    int show_help;
} options;
/////////////////////////////////////////////////////////////////////////////
// FUSE option specification
static const struct fuse_opt option_spec[] = {
    {"-h", offsetof(struct options, show_help), 1},
    {"--help", offsetof(struct options, show_help), 1},
    {"--volume_label=%s", offsetof(struct options, volume_label), 1},
    FUSE_OPT_END};
/////////////////////////////////////////////////////////////////////////////
// Function to display help information
static void show_help(const char *progname) {
    printf("---------------------------------------------------------------------------------------------------\n");
    printf("Instructions on HOW TO USE memefs ---> ./%s <mount_point> <image_file> [options]\n", progname);
    printf("\n");
    printf("OPTIONS:\n");
    printf("  --volume_label=\"content\"  <-- Set the volume label')\n");
    printf("  -h, --help           <-- Show this help message\n");
    printf("---------------------------------------------------------------------------------------------------\n");
}
/////////////////////////////////////////////////////////////////////////////
// the implemented file operations functions
const struct fuse_operations driver_operations = {
    .getattr = driver_getattr,
    .readdir = driver_readdir,
    .create  = driver_create,
    .unlink  = driver_unlink,
    .open    = driver_open,
    .read    = driver_read,
    .write   = driver_write,
    .truncate = driver_truncate,
    .utimens = driver_utimens,
};
/////////////////////////////////////////////////////////////////////////////
// Main, initializes the FUSE filesystem and mounts to the directory
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mount_point> <image_file>\n", argv[0]);
        return 1;}
    // Save last 2 arguments as mountpoint and image file
    const char *mount_point = argv[argc-2];
    const char *image_file = argv[argc-1];
    printf("mountpoint=%s\n", mount_point);
    printf("image_file=%s\n", image_file);

    // Parse args 
    struct fuse_args args = FUSE_ARGS_INIT(argc-1, argv);
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1) return 1;
    // Initialize driver and load image file
    init_driver(image_file, options.volume_label);
    // Pass control to FUSE
    int ret = fuse_main(args.argc, args.argv, &driver_operations, NULL);

    // free memory
    fuse_opt_free_args(&args);
    free((void *)options.volume_label);
    // Unload the image file after unmounting

    cleanup_driver();
    return ret;
}
