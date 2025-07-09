#ifndef OPINION_SPACE_H
#define OPINION_SPACE_H

#include "../01-graph/graph.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct opinion_space {
	// Core data storage
	void *opinions;
	size_t num_agents;
	size_t element_size;

	// Accessor method (const-safe)
	const void *(*get_opinion)(const struct opinion_space * os,
				   int index);

	// Setter method
	bool (*set_opinion)(struct opinion_space * os, int index,
			    const void *value);

	// Distance metric (NULL for default Euclidean)
	void *(*get_distance)(const void *opinion1, const void *opinion2);
} opinion_space;

// Memory management
void free_opinion_space(opinion_space * os);

// Space creation functions
opinion_space *create_opinion_space(size_t num_agents,
				    size_t element_size);

opinion_space *create_finite_domain_space(size_t num_agents,
					  const void *domain_samples,
					  size_t element_size,
					  void (*sampler)(struct
							  opinion_space *
							  os,
							  int target_idx,
							  const void
							  *domain)
    );

// Type-safe access macros
#define OPINION_AS(type, os, index) (*((const type*)((os)->get_opinion((os), (index)))))
#define SET_OPINION(type, os, index, value) do { \
    const type __tmp = (value); \
    (os)->set_opinion((os), (index), &__tmp); \
} while(0)

#endif				// OPINION_SPACE_H
