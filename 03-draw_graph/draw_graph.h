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
#define NODE_RADIUS 50
#define MAX_PLACEMENT_ATTEMPTS 1000

// RGB color struct
typedef struct {
    float r, g, b;
} RGBColor;

// Visualization node position struct
typedef struct {
    double x, y;
} VisNode;

/**
 * Free the allocated layout nodes array.
 */
void free_layout(VisNode **layout_ptr);

/**
 * Check if a position (x,y) is valid, i.e., not too close to previously placed nodes.
 */
int position_ok(VisNode *nodes, int placed_nodes, double x, double y);

/**
 * Assign random positions to nodes within WIDTH x HEIGHT, avoiding overlaps.
 */
int assign_random_layout(VisNode *nodes, int n);

/**
 * Save the layout (node positions) to a file.
 */
int save_layout_to_path(const char *path, VisNode *layout, size_t n);

/**
 * Load the layout (node positions) from a file.
 */
int load_layout_from_path(const char *path, VisNode **layout_ptr, size_t n);

/**
 * Save the graph visualization as a PNG image.
 * If layout is NULL, a random layout is assigned internally.
 * colors can be NULL; if provided, must be length num_nodes.
 * labels can be NULL; if provided, must be length num_nodes.
 */
void save_graph_as_image(graph *g, RGBColor *colors, const char *filename, char **labels,
                         VisNode *layout);

/**
 * Compute a spring layout for the graph.
 * If prev_layout is NULL, start from random positions.
 * Otherwise, start from prev_layout positions for smoother transitions.
 * Returns a newly allocated VisNode array with positions.
 */
//VisNode* compute_spring_layout(graph *g, VisNode *prev_layout, int num_iters, float width, float height);

/**
 * Save the graph visualization as a PNG image using a spring layout.
 * prev_layout can be NULL to start fresh.
 * The function computes the spring layout internally.
 */
//void spring_save_graph_as_image(graph *g, RGBColor *colors, const char *filename,char **labels, VisNode *prev_layout);

#endif // GRAPH_VISUALIZATION_H
