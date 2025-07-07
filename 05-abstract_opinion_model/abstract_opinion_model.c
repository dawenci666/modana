#include "abstract_opinion_model.h"
#include <stdlib.h>

opinion_model* create_model(
    graph* network,
    opinion_space* opinion_space,
    void* params,
    void (*update_fn)(opinion_model* model)
) {
    if (!network || !opinion_space || !update_fn) return NULL;

    opinion_model* model = malloc(sizeof(opinion_model));
    if (!model) return NULL;

    model->network = network;
    model->opinion_space = opinion_space;
    model->params = params;          // just store the pointer, no copy
    model->update = update_fn;
    return model;
}

void free_model(opinion_model* model) {
    if (!model) return;
    free(model);
}
