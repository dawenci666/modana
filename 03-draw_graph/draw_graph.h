#ifndef GRAPH_VISUALIZATION_H
#define GRAPH_VISUALIZATION_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <libgen.h>
#include <stddef.h>  // For dirname
#include <cairo.h>
#include "../01-graph/graph.h"

#define WIDTH 1920
#define HEIGHT 1080
#define MAX_PLACEMENT_ATTEMPTS 1000

// RGB color struct
typedef struct {
    float r, g, b;
} RGBColor;

// Visualization node position struct
typedef struct {
    double x, y;
} VisNode;

typedef struct {
    int** clusters;
    int* cluster_sizes;
    int num_clusters;
    float* avg_opinions;   // NEW: average opinion per cluster
} cluster_result;

void free_layout(VisNode **layout_ptr);

int assign_random_layout(VisNode *nodes, float *node_sizes, int n);  // ✅ ADDED node_sizes

int save_layout_to_path(const char *path, VisNode *layout, size_t n);

int load_layout_from_path(const char *path, VisNode **layout_ptr, size_t n);

void save_graph_as_image(graph *g, RGBColor *node_colors, RGBColor *edge_colors, float *node_sizes,  // ✅ ADDED node_sizes
                         const char *filename, char **labels, VisNode *layout, cluster_result* clusters);

#endif // GRAPH_VISUALIZATION_H
