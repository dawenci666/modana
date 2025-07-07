#ifndef GRAPH_GENERATORS_H
#define GRAPH_GENERATORS_H

#include "../01-graph/graph.h"

// Erdős-Rényi random graph
graph* generate_erdos_renyi(int n, float p, int is_directed);

// Watts-Strogatz small world network
graph* generate_small_world(int n, int k, float beta, int is_directed);

// Barabási-Albert scale-free network
graph* generate_scale_free(int n, int m, int is_directed);

#endif