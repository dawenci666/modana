#include "create_dir_with_curr_timestamp.h"

// Returns a newly allocated string with the path of the created directory
// Caller is responsible for free() of the returned string
char *create_dir_with_curr_timestamp(const char *input_dir)
{
	if (!input_dir || strlen(input_dir) == 0) {
		fprintf(stderr, "Invalid input directory.\n");
		return NULL;
	}
	// Prepare timestamp string
	time_t now = time(NULL);
	if (now == (time_t) - 1) {
		perror("time() failed");
		return NULL;
	}

	struct tm *tm_info = localtime(&now);
	if (!tm_info) {
		perror("localtime() failed");
		return NULL;
	}

	char timestamp[64];
	if (strftime
	    (timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S",
	     tm_info) == 0) {
		fprintf(stderr, "strftime failed\n");
		return NULL;
	}
	// Allocate buffer for new directory path: input_dir + '/' + timestamp + '\0'
	size_t len = strlen(input_dir) + 1 + strlen(timestamp) + 1;
	char *generated_dir = malloc(len);
	if (!generated_dir) {
		perror("malloc failed");
		return NULL;
	}
	// Compose full path
#ifdef _WIN32
	// On Windows, use backslash
	snprintf(generated_dir, len, "%s\\%s", input_dir, timestamp);
#else
	// On Unix, use forward slash
	snprintf(generated_dir, len, "%s/%s", input_dir, timestamp);
#endif

	// Create the directory
	if (mkdir_safe(generated_dir) != 0) {
		if (errno != EEXIST) {
			perror("mkdir failed");
			free(generated_dir);
			return NULL;
		}
		// Directory already exists - can be considered success
	}

	return generated_dir;
}
