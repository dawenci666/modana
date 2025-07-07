#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

// Helper: Check if a path is a directory
int is_directory(const char *path) {
    struct stat s;
    return (stat(path, &s) == 0 && S_ISDIR(s.st_mode));
}

// Helper: Comparator for qsort
int compare_filenames(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void generate_video_from_images(const char *directoryname, const char *video_filename, int fps) {
    char images_path[PATH_MAX];
    snprintf(images_path, sizeof(images_path), "%s/images", directoryname);

    // Check if images directory exists
    if (!is_directory(images_path)) {
        printf("No images folder found\n");
        return;
    }

    // Open images directory
    DIR *dir = opendir(images_path);
    if (!dir) {
        perror("Failed to open images directory");
        return;
    }

    // Read image filenames
    char **filenames = NULL;
    int count = 0, capacity = 10;
    filenames = malloc(sizeof(char *) * capacity);

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (
                    strcasecmp(ext, ".png") == 0 ||
                    strcasecmp(ext, ".jpg") == 0 ||
                    strcasecmp(ext, ".jpeg") == 0)) {
                if (count >= capacity) {
                    capacity *= 2;
                    filenames = realloc(filenames, sizeof(char *) * capacity);
                }
                filenames[count++] = strdup(entry->d_name);
            }
        }
    }
    closedir(dir);

    if (count == 0) {
        printf("No image files found in images folder\n");
        free(filenames);
        return;
    }

    // Sort filenames
    qsort(filenames, count, sizeof(char *), compare_filenames);

    // Default fps if not set
    if (fps <= 0)
        fps = 3;

    // Default video filename
    char full_output_path[PATH_MAX];
    if (!video_filename) {
        snprintf(full_output_path, sizeof(full_output_path), "%s/animation.mp4", directoryname);
    } else {
        snprintf(full_output_path, sizeof(full_output_path), "%s", video_filename);
    }

    // Create temporary file list for ffmpeg
    char list_file[PATH_MAX];
    snprintf(list_file, sizeof(list_file), "%s/imagelist.txt", directoryname);
    FILE *f = fopen(list_file, "w");
    if (!f) {
        perror("Failed to create image list file");
        goto cleanup;
    }

    // ✅ FIXED: Write relative paths (relative to the directory that holds images)
    for (int i = 0; i < count; i++) {
        fprintf(f, "file 'images/%s'\n", filenames[i]);  // Use relative paths
        free(filenames[i]);
    }
    fclose(f);

    // Construct and run ffmpeg command
    char cmd[4 * PATH_MAX];
snprintf(cmd, sizeof(cmd),
         "ffmpeg -y -r %d -f concat -safe 0 -i \"%s\" -vf \"scale=1920:1080:force_original_aspect_ratio=decrease,pad=1920:1080:(ow-iw)/2:(oh-ih)/2\" -c:v libx264 -pix_fmt yuv420p \"%s\"",
         fps, list_file, full_output_path);

    printf("Running: %s\n", cmd);
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "ffmpeg failed with code %d\n", result);
    } else {
        printf("Video created at: %s\n", full_output_path);
    }

    // Clean up
    remove(list_file);

cleanup:
    free(filenames);
}
