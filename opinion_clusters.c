#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "01-graph/graph.h"  // your graph header

#define MAX_NODES 1024

typedef struct {
    int** clusters;
    int* cluster_sizes;
    int num_clusters;
    float* avg_opinions;   // NEW: average opinion per cluster
} cluster_result;

static bool is_valid_opinion(float* opinions, int* cluster, int size, int node, float max_op_diff) {
    for (int i = 0; i < size; i++) {
        if (fabsf(opinions[cluster[i]] - opinions[node]) > max_op_diff)
            return false;
    }
    return true;
}

cluster_result find_disjoint_maximal_opinion_clusters(graph* g, float* opinions, float max_distance, float max_op_diff) {
    int n = g->num_nodes;
    float* distances = compute_all_pairs_distances(g);

    cluster_result result = {0};
    result.clusters = malloc(sizeof(int*) * n);
    result.cluster_sizes = malloc(sizeof(int) * n);
    result.avg_opinions = malloc(sizeof(float) * n);   // allocate avg_opinions array
    result.num_clusters = 0;

    bool visited[MAX_NODES] = {false};

    for (int start = 0; start < n; start++) {
        if (visited[start]) continue;

        int* cluster = malloc(sizeof(int) * n);
        int cluster_size = 0;
        int queue[MAX_NODES], front = 0, rear = 0;

        queue[rear++] = start;

        while (front < rear) {
            int curr = queue[front++];
            if (visited[curr]) continue;

            visited[curr] = true;
            cluster[cluster_size++] = curr;

            for (int neighbor = 0; neighbor < n; neighbor++) {
                if (visited[neighbor]) continue;

                float d = distances[start * n + neighbor];
                if (d <= max_distance && is_connected(g, curr, neighbor) &&
                    is_valid_opinion(opinions, cluster, cluster_size, neighbor, max_op_diff)) {
                    queue[rear++] = neighbor;
                }
            }
        }

        // Compute average opinion for this cluster
        float sum_op = 0.0f;
        for (int i = 0; i < cluster_size; i++) {
            sum_op += opinions[cluster[i]];
        }
        float avg_op = (cluster_size > 0) ? sum_op / cluster_size : 0.0f;

        result.clusters[result.num_clusters] = cluster;
        result.cluster_sizes[result.num_clusters] = cluster_size;
        result.avg_opinions[result.num_clusters] = avg_op;  // store avg opinion
        result.num_clusters++;
    }

    free(distances);
    return result;
}

cluster_result find_overlapping_maximal_opinion_clusters(graph* g, float* opinions, float max_distance, float max_op_diff) {
    int n = g->num_nodes;
    float* distances = compute_all_pairs_distances(g);

    cluster_result result = {0};
    result.clusters = malloc(sizeof(int*) * n);
    result.cluster_sizes = malloc(sizeof(int) * n);
    result.avg_opinions = malloc(sizeof(float) * n);   // allocate avg_opinions array
    result.num_clusters = 0;

    for (int start = 0; start < n; start++) {
        bool in_cluster[MAX_NODES] = {false};
        int* cluster = malloc(sizeof(int) * n);
        int cluster_size = 0;
        int queue[MAX_NODES], front = 0, rear = 0;

        queue[rear++] = start;

        while (front < rear) {
            int curr = queue[front++];
            if (in_cluster[curr]) continue;

            in_cluster[curr] = true;
            cluster[cluster_size++] = curr;

            for (int neighbor = 0; neighbor < n; neighbor++) {
                if (in_cluster[neighbor]) continue;

                float d = distances[start * n + neighbor];
                if (d <= max_distance && is_connected(g, curr, neighbor) &&
                    is_valid_opinion(opinions, cluster, cluster_size, neighbor, max_op_diff)) {
                    queue[rear++] = neighbor;
                }
            }
        }

        if (cluster_size > 1) {
            // Compute average opinion for this cluster
            float sum_op = 0.0f;
            for (int i = 0; i < cluster_size; i++) {
                sum_op += opinions[cluster[i]];
            }
            float avg_op = (cluster_size > 0) ? sum_op / cluster_size : 0.0f;

            result.clusters[result.num_clusters] = cluster;
            result.cluster_sizes[result.num_clusters] = cluster_size;
            result.avg_opinions[result.num_clusters] = avg_op;  // store avg opinion
            result.num_clusters++;
        } else {
            free(cluster);
        }
    }

    free(distances);
    return result;
}

void free_cluster_result(cluster_result* result) {
    for (int i = 0; i < result->num_clusters; i++) {
        free(result->clusters[i]);
    }
    free(result->clusters);
    free(result->cluster_sizes);
    free(result->avg_opinions);  // free avg_opinions array
}
