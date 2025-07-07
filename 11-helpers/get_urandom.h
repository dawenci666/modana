// get_urandom.h
#ifndef GET_URANDOM_H
#define GET_URANDOM_H
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
// Returns a random float between min and max using /dev/urandom.
// On error, returns 0.0f.
float get_urandom(float min, float max);

// Optional: function to close /dev/urandom file descriptor to clean up.
// Call this when you are done using get_urandom to avoid resource leaks.
void close_urandom();

#endif // GET_URANDOM_H
