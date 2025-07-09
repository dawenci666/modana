#include "abstract_opinion_space.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

const void *default_get(const opinion_space *os, int idx)
{
	assert(idx >= 0 && (size_t)idx < os->num_agents);
	return (char *)os->opinions + idx * os->element_size;
}

static bool default_set(opinion_space *os, int idx, const void *val)
{
	if (idx < 0 || (size_t)idx >= os->num_agents)
		return false;
	memcpy((char *)os->opinions + idx * os->element_size,
	       val, os->element_size);
	return true;
}

static void *default_distance(const void *opinion1, const void *opinion2)
{
	float *a = (float *)opinion1;
	float *b = (float *)opinion2;

	float diff = fabsf(*a - *b);

	float *result = malloc(sizeof(float));
	if (!result)
		return NULL;

	*result = diff;
	return result;
}

opinion_space *create_opinion_space(size_t num, size_t esize)
{
	opinion_space *os = calloc(1, sizeof(opinion_space));
	if (!os)
		return NULL;

	os->opinions = calloc(num, esize);
	if (!os->opinions) {
		free(os);
		return NULL;
	}

	os->num_agents = num;
	os->element_size = esize;
	os->get_opinion = default_get;
	os->set_opinion = default_set;
	os->get_distance = default_distance;

	return os;
}

opinion_space *create_finite_domain_space(size_t num_agents,
					  const void *domain_samples,
					  size_t element_size,
					  void (*sampler)(struct
							  opinion_space
							  *os,
							  int target_idx,
							  const void
							  *domain)
    )
{
	opinion_space *os = create_opinion_space(num_agents, element_size);
	if (!os)
		return NULL;

	for (int i = 0; i < (int)num_agents; i++) {
		sampler(os, i, domain_samples);
	}

	return os;
}

void free_opinion_space(opinion_space *os)
{
	if (os) {
		free(os->opinions);
		free(os);
	}
}
