#include "social_impact_model.h"

typedef struct {
    float alpha;
    float beta;
    float* distances;        // flattened size n * n
    float* persuasiveness;   // size n
    float* support;          // size n
} social_impact_params;

float mult_impact_i(size_t i, social_impact_params* params, float* os, size_t num_nodes)
{
    float impact = 0.0f;

    for (size_t j = 0; j < num_nodes; j++) {
        float dij_alpha = powf(params->distances[num_nodes * i + j], params->alpha);
        impact += (params->persuasiveness[j] / dij_alpha) * (1 - os[i] * os[j])
                  - (params->support[j] / dij_alpha) * (1 + os[i] * os[j]);
    }

    return impact;
}

void social_impact_async_mult_update(opinion_model* model)
{
    float* opinions = (float*)model->opinion_space->opinions;
    size_t n = model->network->num_nodes;
    social_impact_params* params = (social_impact_params*)model->params;
    float beta = params->beta;

    for (size_t i = 0; i < n; ++i) {
        float old_opinion = opinions[i];
        float impact = mult_impact_i(i, params, opinions, n);
        opinions[i] = tanhf(old_opinion * beta * impact);
    }
}

opinion_model* create_si_async_mult_model(graph* topology,
    float alpha, float beta, float* distances)
{
    if (!topology)
        return NULL;
    opinion_model* model = malloc(sizeof(opinion_model));
    if (!model)
        return NULL;

    social_impact_params* params = malloc(sizeof(social_impact_params));
    if (!params) {
        free(model);
        return NULL;
    }

    params->alpha = alpha;
    params->beta = beta;
    if(!distances){
        params->distances = malloc(topology->num_nodes*topology->num_nodes*sizeof(float));
        for(size_t i=0;i<topology->num_nodes*topology->num_nodes;i++){
            params->distances[i]  = 1;
        }
    }
    params->persuasiveness = create_opinions_in_real_ball_of_radius_one(topology->num_nodes)->opinions;
    params->support = create_opinions_in_real_ball_of_radius_one(topology->num_nodes)->opinions;

    if (!params->persuasiveness || !params->support) {
        free(params->persuasiveness);
        free(params->support);
        free(params);
        free(model);
        return NULL;
    }

    model->network = topology;
    model->opinion_space = create_opinions_in_real_ball_of_radius_one(topology->num_nodes);

    if (!model->opinion_space) {
        free(params->persuasiveness);
        free(params->support);
        free(params);
        free(model);
        return NULL;
    }

    model->params = params;
    model->update = social_impact_async_mult_update;

    return model;
}