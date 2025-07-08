#ifndef GRAPH_GENERATORS_H
#define GRAPH_GENERATORS_H

#include "../01-graph/graph.h"

// Erdős-Rényi random graph generator
graph* generate_erdos_renyi(int n, float p, int is_directed);

// Watts-Strogatz small-world network generator
// k must be even
graph* generate_watts_strogatz(int n, int k, float beta, int is_directed);

// Barabási-Albert scale-free network generator
graph* generate_barabasi_albert(int n, int m, int is_directed);

#endif // GRAPH_GENERATORS_H
