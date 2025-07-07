#ifndef OPINION_MODEL_H
#define OPINION_MODEL_H

#include "../01-graph/graph.h"
#include "../04-abstract_opinion_space/abstract_opinion_space.h"
#include <stddef.h>

typedef struct opinion_model {
    graph* network;
    opinion_space* opinion_space;
    void* params;         // Pointer to a block of memory holding model parameters/ Size of the params block in bytes
    void (*update)(struct opinion_model* model);  // Model-specific update function
} opinion_model;

// Create model - params pointer is copied, ownership stays with caller (or you can copy inside)
opinion_model* create_model(
    graph* network,
    opinion_space* opinion_space,
    void* params,
    void (*update_fn)(struct opinion_model* model)
);

void free_model(opinion_model* model);
#endif // OPINION_MODEL_H
