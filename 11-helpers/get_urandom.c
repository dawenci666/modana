// get_urandom.c
#include "get_urandom.h"

static int urandom_fd = -1;

static int open_urandom() {
    if (urandom_fd == -1) {
        urandom_fd = open("/dev/urandom", O_RDONLY);
        if (urandom_fd == -1) {
            perror("open /dev/urandom");
            return -1;
        }
    }
    return 0;
}

float get_urandom(float min, float max) {
    if (open_urandom() == -1) {
        return 0.0f;  // fallback on error
    }

    uint32_t rand_val;
    ssize_t result = read(urandom_fd, &rand_val, sizeof(rand_val));
    if (result != sizeof(rand_val)) {
        perror("read /dev/urandom");
        return 0.0f;
    }

    float normalized = (float)rand_val / (float)UINT32_MAX;
    return min + normalized * (max - min);
}

void close_urandom() {
    if (urandom_fd != -1) {
        close(urandom_fd);
        urandom_fd = -1;
    }
}
