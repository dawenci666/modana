#include "draw_graph_opinion_labels.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "01-graph/graph.h"  // your graph header

#define MAX_NODES 1024



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


char **build_opinion_labels(const float *opinions, size_t num_nodes) {
    char **labels = malloc(sizeof(char *) * num_nodes);
    if (!labels)
        return NULL;

    for (size_t i = 0; i < num_nodes; i++) {
        labels[i] = malloc(MAX_LABEL_LEN);
        if (!labels[i]) {
            for (size_t j = 0; j < i; j++)
                free(labels[j]);
            free(labels);
            return NULL;
        }
        snprintf(labels[i], MAX_LABEL_LEN, "%zu - %.3f", i, opinions[i]);
    }
    return labels;
}

void free_labels(char **labels, size_t num_labels) {
    if (!labels)
        return;
    for (size_t i = 0; i < num_labels; i++) {
        free(labels[i]);
    }
    free(labels);
}

float *read_opinions(const char *filename, size_t *out_num_nodes) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open opinions file");
        return NULL;
    }

    size_t count = 0;
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n')
            count++;
    }
    rewind(file);

    float *opinions = calloc(count, sizeof(float));
    if (!opinions) {
        fclose(file);
        return NULL;
    }

    size_t idx;
    float val;
    while (fscanf(file, "%zu %f\n", &idx, &val) == 2) {
        if (idx >= count) {
            fprintf(stderr, "Warning: node index %zu out of range in opinions file\n", idx);
            continue;
        }
        opinions[idx] = val;
    }
    fclose(file);

    *out_num_nodes = count;
    return opinions;
}

char *path_layout_file(const char *directory) {
    size_t len = strlen(directory) + strlen("/graph.layout") + 1;
    char *buf = malloc(len);
    if (!buf) return NULL;
    snprintf(buf, len, "%s/graph.layout", directory);
    return buf;
}

void save_image_as_graph_wopinion_labels_and_colors(char *graph_filename,
                                                    char *opinion_filename,
                                                    char *imgfilename,
                                                    char *layout_file,
                                                    int num_nodes_hint)
{
    graph *g = read_graph(graph_filename);
    if (!g) {
        fprintf(stderr, "Failed to read graph from %s\n", graph_filename);
        return;
    }

    size_t num_nodes = 0;
    float *opinions = read_opinions(opinion_filename, &num_nodes);
    if (!opinions) {
        fprintf(stderr, "Failed to read opinions from %s\n", opinion_filename);
        free_graph(g);
        return;
    }

    RGBColor *node_colors = malloc(sizeof(RGBColor) * num_nodes);
    RGBColor *edge_colors = malloc(sizeof(RGBColor) * num_nodes * num_nodes);
    float *node_sizes = malloc(sizeof(float) * num_nodes); 

    float mellow = 0.6f;

for (size_t i = 0; i < num_nodes; i++) {
    float val = opinions[i];
    int r = 0, g_col = 0, b = 0;

    if (val <= 0.0f) {
        // -1 to 0 → red to green
        float t = val + 1.0f;  // t ∈ [0, 1]
        r = (int)(180 * (1.0f - t));  // max 180 red
        g_col = (int)(180 * t);       // max 180 green
        // b = 0
    } else {
        // 0 to 1 → green to blue
        float t = val;               // t ∈ [0, 1]
        g_col = (int)(180 * (1.0f - t));  // green fades out
        b = (int)(180 * t);              // blue fades in
        // r = 0
    }

    node_colors[i] = (RGBColor){ r, g_col, b };
        node_sizes[i] = 50+30*fabsf(opinions[i]);  // or assign dynamically if desired
    }

for (size_t i = 0; i < num_nodes; i++) {
    for (size_t j = 0; j < num_nodes; j++) {
        if (!g->edges[i * num_nodes + j]) continue;

        float opinion_avg = (opinions[i] + opinions[j]) / 2.0f;  // Range: [-1, 1]
        int r = 0, g_col = 0, b = 0;

        if (opinion_avg <= 0.0f) {
            // -1 to 0 → mellow red to mellow green
            float t = opinion_avg + 1.0f;  // t ∈ [0, 1]
            r = (int)(180 * (1.0f - t));   // red fades out
            g_col = (int)(180 * t);        // green fades in
        } else {
            // 0 to 1 → mellow green to mellow blue
            float t = opinion_avg;        // t ∈ [0, 1]
            g_col = (int)(180 * (1.0f - t));  // green fades out
            b = (int)(180 * t);               // blue fades in
        }

        edge_colors[i * num_nodes + j] = (RGBColor){ r, g_col, b };
    }
}


    char **labels = build_opinion_labels(opinions, num_nodes);
    if (!labels) {
        fprintf(stderr, "Failed to build labels for %zu nodes\n", num_nodes);
        goto cleanup;
    }

    VisNode *layout = NULL;
    if (!load_layout_from_path(layout_file, &layout, num_nodes)) {
        layout = malloc(sizeof(VisNode) * num_nodes);
        if (!assign_random_layout(layout, node_sizes, num_nodes)) {  // ✅ Updated call
            fprintf(stderr, "Failed to assign layout\n");
            goto cleanup;
        }
        save_layout_to_path(layout_file, layout, num_nodes);
    }
    cluster_result clusters = find_overlapping_maximal_opinion_clusters(g,opinions,3,0.5);

    save_graph_as_image(g, node_colors, edge_colors, node_sizes, imgfilename, labels, layout,&clusters);  // ✅ Corrected args
cleanup:
    free_cluster_result(&clusters);
    free_graph(g);
    free(opinions);
    free_labels(labels, num_nodes);
    free(node_colors);
    free(edge_colors);
    free(node_sizes);  // ✅ Free node sizes
    free_layout(&layout);
}
