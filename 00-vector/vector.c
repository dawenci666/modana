#include "vector.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

vector* create_vector(size_t dim, const float values[]) {
    vector* v = malloc(sizeof(vector));
    if (!v) return NULL;

    v->dimension = dim;
    v->data = malloc(dim * sizeof(float));
    
    if (!v->data) {
        free(v);
        return NULL;
    }

    if (values) {
        memcpy(v->data, values, dim * sizeof(float));
    } else {
        memset(v->data, 0, dim * sizeof(float));
    }
    
    return v;
}

void free_vector(vector* v) {
    if (v) {
        free(v->data);
        free(v);
    }
}

float vector_get(const vector* v, size_t index) {
    assert(index < v->dimension);
    return ((float*)v->data)[index];
}

void vector_set(vector* v, size_t index, float value) {
    assert(index < v->dimension);
    ((float*)v->data)[index] = value;
}