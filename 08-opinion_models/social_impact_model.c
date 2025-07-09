#include "social_impact_model.h"
#include "../11-helpers/get_urandom.h"
#include "../01-graph/graph.h"
#include<string.h>
#define INF 1e9f

typedef struct {
	float alpha;
	float beta;
	float *distances;	// flattened size n * n
	float *persuasiveness;	// size n
	float *support;		// size n
} social_impact_params;

void free_params(opinion_model *sim)
{
	social_impact_params *params =
	    (social_impact_params *) sim->params;
	free(params->distances);
	free(params->persuasiveness);
	free(params->support);
	free(params);
}

float mult_impact_i(size_t i, social_impact_params *params, float *os,
		    size_t num_nodes)
{
	float impact = 0.0f;

	for (size_t j = 0; j < num_nodes; j++) {
		float dist = params->distances[num_nodes * i + j];
		if (i == j || dist >= INF * 0.9f)
			continue;
		// Prevent zero or very small distances to avoid division by zero
		if (dist < 1e-6f)
			dist = 1e-6f;
		float dij_alpha = powf(dist, params->alpha);
		impact +=
		    (params->persuasiveness[j] / dij_alpha) * (1 -
							       os[i] *
							       os[j])
		    - (params->support[j] / dij_alpha) * (1 +
							  os[i] * os[j]);
	}
	return impact;
}

void social_impact_async_mult_update(opinion_model *model)
{
	float *opinions = (float *)model->opinion_space->opinions;
	size_t n = model->network->num_nodes;
	social_impact_params *params =
	    (social_impact_params *) model->params;
	float beta = params->beta;
	size_t i = (size_t)get_urandom(0.0f, (float)n);
	float impact = mult_impact_i(i, params, opinions, n);
	opinions[i] = tanhf(beta * (opinions[i] * impact));
}

opinion_model *create_si_async_mult_model(graph *topology,
					  float alpha, float beta)
{
	if (!topology)
		return NULL;
	opinion_model *model = malloc(sizeof(opinion_model));
	if (!model)
		return NULL;

	social_impact_params *params =
	    malloc(sizeof(social_impact_params));
	if (!params) {
		free(model);
		return NULL;
	}

	params->alpha = alpha;
	params->beta = beta;
	params->distances = compute_all_pairs_distances(topology);
	params->persuasiveness =
	    create_opinions_in_real_ball_of_radius_one(topology->
						       num_nodes)->
	    opinions;
	params->support =
	    create_opinions_in_real_ball_of_radius_one(topology->
						       num_nodes)->
	    opinions;

	if (!params->persuasiveness || !params->support) {
		free(params->persuasiveness);
		free(params->support);
		free(params);
		free(model);
		return NULL;
	}

	model->network = topology;
	model->opinion_space =
	    create_opinions_in_real_ball_of_radius_one(topology->
						       num_nodes);

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

float *compute_all_pairs_opinion_differences(float *opinions,
					     int num_nodes)
{
	float *differences = malloc(sizeof(float) * num_nodes * num_nodes);
	if (!differences)
		return NULL;
	for (int i = 0; i < num_nodes; i++) {
		for (int j = 0; j < num_nodes; j++) {
			differences[i * num_nodes + j] =
			    fabsf(opinions[i] - opinions[j]);
		}
	}
	return differences;
}

float return_max(float *matrix, size_t size)
{
	if (!matrix)
		return -INFINITY;	// or some error value
	float max = matrix[0];	// start with first element
	for (size_t i = 1; i < size; i++) {
		if (matrix[i] > max)
			max = matrix[i];
	}
	return max;
}

void apply_natural_decay(graph *topology, float decay_rate,
			 float min_bond_strength)
{
	int num_nodes = topology->num_nodes;
	for (int i = 0; i < num_nodes; i++) {
		for (int j = 0; j < num_nodes; j++) {
			if (i == j)
				continue;
			if (!topology->edges[i * num_nodes + j])
				continue;
			float edge_weight =
			    topology->edge_weights[i * num_nodes + j];
			float bond_strength = 1.0f - edge_weight;
			// Apply decay to bond strength
			bond_strength *= (1.0f - decay_rate);
			// Remove edge if bond strength below threshold
			if (bond_strength < min_bond_strength) {
				bond_strength = 0.0f;
				topology->edges[i * num_nodes + j] = 0;	// Remove edge
			}
			edge_weight = 1.0f - bond_strength;
			// Clamp edge weight between 0 and 1
			if (edge_weight < 0.0f)
				edge_weight = 0.0f;
			if (edge_weight > 1.0f)
				edge_weight = 1.0f;
			topology->edge_weights[i * num_nodes + j] =
			    edge_weight;
			if (topology->is_directed)
				topology->edge_weights[i * num_nodes + j] =
				    topology->edge_weights[j * num_nodes +
							   i];
		}
	}
}

void update_topology_homophily(graph *topology, float *opinions,
			       const float opinion_similarity_threshold,
			       const float bond_reinforcement_rate,
			       const float bond_weakening_rate,
			       const float minimum_bond_strength)
{
	int num_nodes = topology->num_nodes;

	for (int node_i = 0; node_i < num_nodes; node_i++) {
		for (int node_j = 0; node_j < num_nodes; node_j++) {
			if (node_i == node_j)
				continue;
			if (!topology->edges[node_i * num_nodes + node_j])
				continue;

			float opinion_difference =
			    fabsf(opinions[node_i] - opinions[node_j]);
			float edge_weight =
			    topology->edge_weights[node_i * num_nodes +
						   node_j];
			float bond_strength = 1.0f - edge_weight;

			// Reinforce bond if opinions are similar
			if (opinion_difference <
			    opinion_similarity_threshold) {
				float reinforcement_amount =
				    bond_reinforcement_rate * (1.0f -
							       opinion_difference)
				    * (1.0f - bond_strength);
				bond_strength += reinforcement_amount;
			} else {
				float weakening_amount =
				    bond_weakening_rate *
				    opinion_difference * bond_strength;
				bond_strength -= weakening_amount;
			}

			// Clamp bond strength and remove edge if below threshold
			if (bond_strength > 1.0f)
				bond_strength = 1.0f;
			if (bond_strength < minimum_bond_strength) {
				bond_strength = 0.0f;
				topology->edges[node_i * num_nodes + node_j] = 0;	// Remove edge
			}
			// Convert bond strength back to edge weight
			edge_weight = 1.0f - bond_strength;
			if (edge_weight < 0.0f)
				edge_weight = 0.0f;
			if (edge_weight > 1.0f)
				edge_weight = 1.0f;

			topology->edge_weights[node_i * num_nodes +
					       node_j] = edge_weight;

			if (topology->is_directed) {
				topology->edge_weights[node_j * num_nodes +
						       node_i] =
				    edge_weight;
			}
		}
	}
}

void create_edges_by_distance_and_opinion_similarity(graph *topology, float *opinions, float *distances,	// shortest path distances, INF if no path
						     float base_creation_probability,	// minimum base prob, > 0
						     float similarity_factor,	// multiplier for opinion similarity effect
						     float distance_factor_scale,	// multiplier for distance effect
						     float
						     initial_bond_strength)
{
	int num_nodes = topology->num_nodes;
	float *differences =
	    compute_all_pairs_opinion_differences(opinions, num_nodes);
	if (!differences)
		return;

	float max_opinion_diff =
	    return_max(differences, num_nodes * num_nodes);
	if (max_opinion_diff == 0)
		max_opinion_diff = 1.0f;	// avoid div by zero

	for (int i = 0; i < num_nodes; i++) {
		for (int j = 0; j < num_nodes; j++) {
			if (i == j)
				continue;
			if (topology->edges[i * num_nodes + j])
				continue;	// Skip existing edges

			float dist = distances[i * num_nodes + j];
			float opinion_diff =
			    differences[i * num_nodes + j];
			float sigma = 0.2f;	// tune this parameter to control width of similarity decay
			float opinion_similarity =
			    expf(-(opinion_diff * opinion_diff) /
				 (sigma * sigma));

			float creation_prob = base_creation_probability;	// start from base

			if (!isinf(dist)) {
				// if path exists, scale creation prob up depending on distance (closer -> higher prob)
				float distance_factor = 1.0f / (1.0f + dist);	// distance factor decreases with dist
				creation_prob +=
				    base_creation_probability *
				    distance_factor_scale *
				    distance_factor * similarity_factor *
				    opinion_similarity;
			}
			// else: no path => creation_prob = base_creation_probability (already set)
			float rand_val = get_urandom(0, 1);
			if (rand_val < creation_prob) {
				topology->edges[i * num_nodes + j] = 1;
				topology->edge_weights[i * num_nodes + j] =
				    initial_bond_strength;

				if (!topology->is_directed) {
					topology->edges[j * num_nodes +
							i] = 1;
					topology->edge_weights[j *
							       num_nodes +
							       i] =
					    initial_bond_strength;
				}
				//printf("Created edge between %d and %d with probability %.3f\n", i, j, creation_prob);
			}
		}
	}

	free(differences);
}

void update_topology_mixed(graph *topology,
			   float *opinions,
			   float *shortest_path_distances,
			   float opinion_similarity_threshold,
			   float bond_reinforcement_rate,
			   float bond_weakening_rate,
			   float decay_rate,
			   float minimum_bond_strength,
			   float base_creation_probability,
			   float similarity_factor,
			   float distance_factor_scale,
			   float initial_bond_strength)
{
	/* Step 1: Apply homophily-based bond reinforcement/weakening */
	update_topology_homophily(topology,
				  opinions,
				  opinion_similarity_threshold,
				  bond_reinforcement_rate,
				  bond_weakening_rate,
				  minimum_bond_strength);

	/* Step 2: Apply natural decay to all existing bonds */
	apply_natural_decay(topology, decay_rate, minimum_bond_strength);

	/* Step 3: Create new edges based on distance and opinion similarity */
	create_edges_by_distance_and_opinion_similarity(topology,
							opinions,
							shortest_path_distances,
							base_creation_probability,
							similarity_factor,
							distance_factor_scale,
							initial_bond_strength);
}

void social_impact_async_mult_update_temporal_topology(opinion_model
						       *model)
{
	float *opinions = (float *)model->opinion_space->opinions;
	size_t n = model->network->num_nodes;
	social_impact_params *params =
	    (social_impact_params *) model->params;
	float beta = params->beta;
	size_t i = (size_t)get_urandom(0.0f, (float)n);
	float impact = mult_impact_i(i, params, opinions, n);
	float noise_strength = 0.01f;	// small magnitude of noise, tune as needed
	float noise = noise_strength * (get_urandom(0, 1) - 0.5f) * 2.0f;
	opinions[i] = tanhf(beta * (impact * opinions[i]));
	float *dist = compute_all_pairs_distances(model->network);
	update_topology_mixed(model->network, opinions, dist, 0.6f,	// opinion_similarity_threshold
			      0.15f,	// bond_reinforcement_rate
			      0.07f,	// bond_weakening_rate
			      0.015f,	// decay_rate
			      0.1f,	// minimum_bond_strength
			      0.005f,	// base_creation_probability
			      4.0f,	// similarity_factor
			      2.0f,	// distance_factor_scale
			      0.5f	// initial_bond_strength
	    );
	free(dist);
}

opinion_model *create_si_async_temporal(graph *topology,
					float alpha, float beta)
{
	if (!topology)
		return NULL;
	opinion_model *model = malloc(sizeof(opinion_model));
	if (!model)
		return NULL;

	social_impact_params *params =
	    malloc(sizeof(social_impact_params));
	if (!params) {
		free(model);
		return NULL;
	}

	params->alpha = alpha;
	params->beta = beta;
	params->distances = compute_all_pairs_distances(topology);
	params->persuasiveness =
	    create_opinions_in_real_ball_of_radius_one(topology->
						       num_nodes)->
	    opinions;
	params->support =
	    create_opinions_in_real_ball_of_radius_one(topology->
						       num_nodes)->
	    opinions;

	if (!params->persuasiveness || !params->support) {
		free(params->persuasiveness);
		free(params->support);
		free(params);
		free(model);
		return NULL;
	}

	model->network = topology;
	model->opinion_space =
	    create_opinions_in_real_ball_of_radius_one(topology->
						       num_nodes);

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
