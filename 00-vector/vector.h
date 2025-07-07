#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

typedef struct {
    void* data;
    size_t dimension;
} vector;

// Creation/destruction
vector* create_vector(size_t dim, const float values[]);
void free_vector(vector* v);

// Accessors
float vector_get(const vector* v, size_t index);
void vector_set(vector* v, size_t index, float value);

#endif