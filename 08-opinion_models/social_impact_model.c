#include "social_impact_model.h"
#include "../11-helpers/get_urandom.h"
#include "../01-graph/graph.h"
#include<string.h>
#define INF 1e9f

typedef struct {
    float alpha;
    float beta;
    float* distances;        // flattened size n * n
    float* persuasiveness;   // size n
    float* support;          // size n
} social_impact_params;

void free_params(opinion_model* sim) {
    social_impact_params* params = (social_impact_params*)sim->params;
    free(params->distances);
    free(params->persuasiveness);
    free(params->support);
    free(params);
}



float mult_impact_i(size_t i, social_impact_params* params, float* os, size_t num_nodes)
{
    float impact = 0.0f;

    for (size_t j = 0; j < num_nodes; j++) {
        float dist = params->distances[num_nodes * i + j];

        // Skip self-distances and unreachable nodes (INF)
        if (i == j || dist >= INF * 0.9f) continue;
        // Prevent zero or very small distances to avoid division by zero
        if (dist < 1e-6f) dist = 1e-6f;
        float dij_alpha = powf(dist, params->alpha);
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
    size_t i = (size_t)get_urandom(0.0f, (float)n);
    float impact = mult_impact_i(i, params, opinions, n);
    opinions[i] = tanhf(opinions[i] * beta * impact);
}

opinion_model* create_si_async_mult_model(graph* topology,
    float alpha, float beta)
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
    params->distances = compute_all_pairs_distances(topology);
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

void update_topology(graph* topology, float* opinions, char* option) {
    int n = topology->num_nodes;

    if (strcmp(option, "homophily") == 0) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                float dist = fabsf(opinions[i] - opinions[j]);
                float w = topology->edge_weights[i*n+j];
                if (dist < 0.3f) {
                    w += 0.05f * (1.0f - dist);
                    if (w > 1.0f) w = 1.0f;
                } else {
                    w -= 0.02f * dist;
                    if (w < 0.0f) w = 0.0f;
                }
                topology->edge_weights[i*n+j] = w;
                if(topology->is_directed) topology->edge_weights[i*n+j] = topology->edge_weights[j*n+i];
            }
        }
    }
    else if (strcmp(option, "avoidance") == 0) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                float dist = fabsf(opinions[i] - opinions[j]);
                float w = topology->edge_weights[i*n+j];
                if (dist < 0.2f) {
                    w -= 0.04f * (0.2f - dist);
                    if (w < 0.0f) w = 0.0f;
                } else {
                    w += 0.03f * (dist - 0.2f);
                    if (w > 1.0f) w = 1.0f;
                }
                topology->edge_weights[i*n+j] = w;
                if(topology->is_directed) topology->edge_weights[i*n+j] = topology->edge_weights[j*n+i];
            }
        }
    }
    else if (strcmp(option, "imitation") == 0) {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                float dist = fabsf(opinions[i] - opinions[j]);
                float w = topology->edge_weights[i*n+j];
                if (dist < 0.15f) {
                    w += 0.06f * (1.0f - dist);
                    if (w > 1.0f) w = 1.0f;
                } else {
                    w -= 0.03f * dist;
                    if (w < 0.0f) w = 0.0f;
                }
                topology->edge_weights[i*n+j] = w;
                if(topology->is_directed) topology->edge_weights[i*n+j] = topology->edge_weights[j*n+i];
            }
        }
    }
    else if (strcmp(option, "payoff-based") == 0) {
        // Payoff-based update logic to be implemented
    }
    else if (strcmp(option, "mixed") == 0) {
        float tau = 0.2f;
        float r = 0.05f;
        float s = 0.03f;
        float delta = 0.01f;
        float kappa = 5.0f;
        float w_init = 0.3f;
        float w_min = 0.05f;

        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                float dist = fabsf(opinions[i] - opinions[j]);
                float w = topology->edge_weights[i*n+j];

                if (w > 0.0f) {
                    if (dist <= tau) {
                        float reinforcement = r * (1.0f - dist) * (1.0f - w);
                        w += reinforcement;
                    } else {
                        float weakening = s * dist * w;
                        w -= weakening;
                    }
                    w *= (1.0f - delta);

                    if (w > 1.0f) w = 1.0f;
                    if (w < 0.0f) w = 0.0f;

                    if (w < w_min) w = 0.0f;
                } else {
                    float p_add = expf(-kappa * dist);
                    if (rand() / (float)RAND_MAX < p_add) {
                        topology->edge_weights[i*n+j] = w_init;
                    }
                }
                topology->edge_weights[i*n+j]  = w;
                if(topology->is_directed) topology->edge_weights[i*n+j] = topology->edge_weights[j*n+i];
            }
        }
    }
}


void social_impact_async_mult_update_temporal_topology(opinion_model* model)
{
    float* opinions = (float*)model->opinion_space->opinions;
    size_t n = model->network->num_nodes;
    social_impact_params* params = (social_impact_params*)model->params;
    float beta = params->beta;
    size_t i = (size_t)get_urandom(0.0f, (float)n);
    float impact = mult_impact_i(i, params, opinions, n);
    opinions[i] = tanhf(opinions[i] * beta * impact);
    char* option = "mixed";
    update_topology(model->network,opinions,option);
}

opinion_model* create_si_async_temporal(graph* topology,
    float alpha, float beta)
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
    params->distances = compute_all_pairs_distances(topology);
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
    model->update = social_impact_async_mult_update_temporal_topology;

    return model;
}