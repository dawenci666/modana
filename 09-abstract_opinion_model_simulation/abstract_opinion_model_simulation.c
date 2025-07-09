#include "abstract_opinion_model_simulation.h"

void write_current_state(opinion_model *model, size_t current_step,
			 const char *directoryname)
{
	if (!directoryname) {
		fprintf(stderr, "Directory name is NULL\n");
		return;
	}

	char filepath[300];
	size_t n = model->network->num_nodes;
	size_t esize = model->opinion_space->element_size;
	float *opinions = (float *)(model->opinion_space->opinions);

	// Write opinions
	snprintf(filepath, sizeof(filepath), "%s/%zu.opinions",
		 directoryname, current_step);
	FILE *opinions_file = fopen(filepath, "w");
	if (!opinions_file) {
		perror("failed to open opinions file");
		return;
	}

	for (size_t i = 0; i < n; i++) {
		float *op = (float *)((char *)opinions + i * esize);
		fprintf(opinions_file, "%zu %.6f\n", i, *op);
	}

	fclose(opinions_file);

	// Write graph
	snprintf(filepath, sizeof(filepath), "%s/%zu.graph", directoryname,
		 current_step);
	save_graph(model->network, filepath);
}

int run_simulation(opinion_model *model, size_t max_steps,
		   float convergence_threshold, const char *directoryname,
		   int save_data)
{
	if (!model) {
		fprintf(stderr, "model is NULL\n");
		return -1;
	}

	if (!model->update) {
		fprintf(stderr, "model->update is NULL\n");
		return -1;
	}

	if (!model->opinion_space) {
		fprintf(stderr, "model->opinion_space is NULL\n");
		return -1;
	}

	if (!model->opinion_space->opinions) {
		fprintf(stderr,
			"model->opinion_space->opinions is NULL\n");
		return -1;
	}

	if (!directoryname || strlen(directoryname) == 0) {
		fprintf(stderr, "Invalid directory name provided.\n");
		return -1;
	}

	size_t n = model->network->num_nodes;
	void *opinions = model->opinion_space->opinions;
	size_t esize = model->opinion_space->element_size;
	int current_step = 0;
	for (size_t step = 0; step < max_steps; step++) {
		model->update(model);

		float max_opinion = -1e9f;
		float min_opinion = 1e9f;

		for (size_t i = 0; i < n; i++) {
			float *current_op =
			    (float *)((char *)opinions + i * esize);
			if (*current_op > max_opinion)
				max_opinion = *current_op;
			if (*current_op < min_opinion)
				min_opinion = *current_op;
		}

		float diff = max_opinion - min_opinion;

		/*printf("Step %zu: ", step);
		   for (size_t i = 0; i < n; i++) {
		   float *op = (float *)((char *)opinions + i * esize);
		   printf("%.6f ", *op);
		   }
		   printf("\nΔ = %.6f\n", diff); */

		if (save_data)
			write_current_state(model, step, directoryname);

		if (diff < convergence_threshold) {
			//printf("Converged after %zu steps (Δ = %.6f).\n", step, diff);
			break;
		}
		current_step = step;
	}
	return current_step;
}
