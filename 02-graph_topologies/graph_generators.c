#include "../01-graph/graph.h"
#include <stdlib.h>
#include <time.h>
#include "../11-helpers/get_urandom.h"  // Add this include

// Helper: get degree of node (out-degree for directed)
static int get_degree(graph* g, int node) {
    int degree = 0;
    int n = g->num_nodes;
    for (int v = 0; v < n; v++) {
        if (is_connected(g, node, v)) degree++;
    }
    return degree;
}

// Check if 'val' is already in 'arr' with length 'len'
static int contains(int* arr, int len, int val) {
    for (int i = 0; i < len; i++) {
        if (arr[i] == val) return 1;
    }
    return 0;
}

graph* generate_erdos_renyi(int n, float p, int is_directed) {
    graph* g = create_graph(n, is_directed);
    if (!g) return NULL;

    for (int u = 0; u < n; u++) {
        for (int v = is_directed ? 0 : u + 1; v < n; v++) {
            if (u != v && get_urandom(0.0f, 1.0f) < p) {
                add_edge(g, u, v, 1.0f);
                if (!is_directed) add_edge(g, v, u, 1.0f);
            }
        }
    }
    return g;
}

graph* generate_small_world(int n, int k, float beta, int is_directed) {
    if (k % 2 != 0) return NULL;

    graph* g = create_graph(n, is_directed);
    if (!g) return NULL;

    int half_k = k / 2;

    // Create ring lattice
    for (int u = 0; u < n; u++) {
        for (int i = 1; i <= half_k; i++) {
            int v = (u + i) % n;
            add_edge(g, u, v, 1.0f);
            if (!is_directed) add_edge(g, v, u, 1.0f);
        }
    }

    // Rewire edges
    for (int u = 0; u < n; u++) {
        for (int i = 1; i <= half_k; i++) {
            int v = (u + i) % n;
            if (is_connected(g, u, v) && get_urandom(0.0f, 1.0f) < beta) {
                // Remove original edge
                g->edges[u * n + v] = 0;
                g->edge_weights[u * n + v] = 0.0f;
                if (!is_directed) {
                    g->edges[v * n + u] = 0;
                    g->edge_weights[v * n + u] = 0.0f;
                }

                // Find new random connection
                int new_v;
                do {
                    new_v = (int)get_urandom(0, (float)n);
                } while (new_v == u || is_connected(g, u, new_v));

                add_edge(g, u, new_v, 1.0f);
                if (!is_directed) add_edge(g, new_v, u, 1.0f);
            }
        }
    }

    return g;
}

graph* generate_scale_free(int n, int m, int is_directed) {
    if (m < 1 || m >= n) return NULL;

    graph* g = create_graph(n, is_directed);
    if (!g) return NULL;

    // Initialize with m+1 fully connected nodes
    int m0 = m + 1;
    for (int u = 0; u < m0; u++) {
        for (int v = u + 1; v < m0; v++) {
            add_edge(g, u, v, 1.0f);
            if (!is_directed) add_edge(g, v, u, 1.0f);
        }
    }

    // Add remaining nodes with preferential attachment
    for (int new_node = m0; new_node < n; new_node++) {
        int* targets = malloc(m * sizeof(int));
        if (!targets) {
            free_graph(g);
            return NULL;
        }

        int total_degree = 0;
        for (int i = 0; i < new_node; i++) {
            total_degree += get_degree(g, i);
        }

        int count = 0;
        while (count < m) {
            int r = (int)get_urandom(0, (float)total_degree);
            int sum = 0;
            int selected = -1;
            for (int i = 0; i < new_node; i++) {
                sum += get_degree(g, i);
                if (sum > r) {
                    selected = i;
                    break;
                }
            }

            // Only add if not duplicate
            if (selected != -1 && !contains(targets, count, selected)) {
                targets[count++] = selected;
            }
            // else pick again
        }

        // Connect new node to targets
        for (int j = 0; j < m; j++) {
            int target = targets[j];
            if (!is_connected(g, new_node, target)) {
                add_edge(g, new_node, target, 1.0f);
                if (!is_directed) add_edge(g, target, new_node, 1.0f);
            }
        }

        free(targets);
    }

    return g;
}
