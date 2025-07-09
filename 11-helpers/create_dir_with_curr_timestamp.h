#ifndef TIMESTAMPED_DIR_H
#define TIMESTAMPED_DIR_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_safe(path) _mkdir(path)
#else
#include <sys/stat.h>
#define mkdir_safe(path) mkdir(path, 0755)
#endif
/**
 * Creates a new directory inside `input_dir` with the current timestamp as its name.
 *
 * @param input_dir The base directory inside which the timestamped directory will be created.
 * @return A newly allocated string with the path to the created directory,
 *         or NULL if creation failed. Caller is responsible for freeing the returned string.
 */
char *create_dir_with_curr_timestamp(const char *input_dir);

#endif				/* TIMESTAMPED_DIR_H */
