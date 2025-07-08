#include "draw_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>
#include "../11-helpers/get_urandom.h"

// --------- Layout Utilities -----------

void free_layout(VisNode **layout_ptr)
{
    if (*layout_ptr) {
        free(*layout_ptr);
        *layout_ptr = NULL;
    }
}
/*
VisNode* compute_spring_layout(graph* g, VisNode* prev_layout, 
                               int num_iters, float width, float height) {
    int n = g->num_nodes;
    float area = width * height;
    float k = sqrtf(area / (float)n);
    float t = width / 10.0f; // initial temperature

    VisNode* layout = malloc(n * sizeof(VisNode));
    if (!layout) {
        fprintf(stderr, "Failed to allocate memory for layout\n");
        return NULL;
    }

    // Initialize positions: from prev_layout if available, else random
    if (prev_layout) {
        for (int i = 0; i < n; i++) {
            layout[i].x = prev_layout[i].x;
            layout[i].y = prev_layout[i].y;
        }
    } else {
        for (int i = 0; i < n; i++) {
            layout[i].x = get_urandom(0.0f, width);
            layout[i].y = get_urandom(0.0f, height);
        }
    }

    float* dx = calloc(n, sizeof(float));
    float* dy = calloc(n, sizeof(float));
    if (!dx || !dy) {
        fprintf(stderr, "Failed to allocate memory for force arrays\n");
        free(layout);
        free(dx);
        free(dy);
        return NULL;
    }

    for (int iter = 0; iter < num_iters; iter++) {
        memset(dx, 0, n * sizeof(float));
        memset(dy, 0, n * sizeof(float));

        // Repulsive forces
        for (int v = 0; v < n; v++) {
            for (int u = v + 1; u < n; u++) {
                float dx_ = layout[v].x - layout[u].x;
                float dy_ = layout[v].y - layout[u].y;
                float dist = sqrtf(dx_ * dx_ + dy_ * dy_) + 0.01f;
                float force = (k * k) / dist;
                float fx = force * dx_ / dist;
                float fy = force * dy_ / dist;
                dx[v] += fx; dy[v] += fy;
                dx[u] -= fx; dy[u] -= fy;
            }
        }

        // Attractive forces
        for (int v = 0; v < n; v++) {
            for (int u = 0; u < n; u++) {
                if (is_connected(g, v, u)) {
                    float dx_ = layout[v].x - layout[u].x;
                    float dy_ = layout[v].y - layout[u].y;
                    float dist = sqrtf(dx_ * dx_ + dy_ * dy_) + 0.01f;
                    float force = (dist * dist) / k;
                    float fx = force * dx_ / dist;
                    float fy = force * dy_ / dist;
                    dx[v] -= fx; dy[v] -= fy;
                    dx[u] += fx; dy[u] += fy;
                }
            }
        }

        // Update positions with cooling
        for (int v = 0; v < n; v++) {
            layout[v].x += fminf(dx[v], t);
            layout[v].y += fminf(dy[v], t);

            // Clamp to bounds
            if (layout[v].x < 0) layout[v].x = 0;
            if (layout[v].x > width) layout[v].x = width;
            if (layout[v].y < 0) layout[v].y = 0;
            if (layout[v].y > height) layout[v].y = height;
        }

        t *= 0.95f;  // cooling
    }

    free(dx);
    free(dy);

    return layout;
}*/

int position_ok(VisNode *nodes, int placed_nodes, double x, double y)
{
    double min_dist = 2 * NODE_RADIUS;
    int i;
    for (i = 0; i < placed_nodes; i++) {
        double dx = nodes[i].x - x;
        double dy = nodes[i].y - y;
        if (hypot(dx, dy) < min_dist)
            return 0;
    }
    return 1;
}

int assign_random_layout(VisNode *nodes, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        int attempts = 0;
        double x, y;

        do {
            // Use get_urandom to generate random positions within bounds
            x = (int)get_urandom(NODE_RADIUS, WIDTH - NODE_RADIUS);
            y = (int)get_urandom(NODE_RADIUS, HEIGHT - NODE_RADIUS);

            attempts++;
            if (attempts > MAX_PLACEMENT_ATTEMPTS)
                return 0;
        } while (!position_ok(nodes, i, x, y));

        nodes[i].x = x;
        nodes[i].y = y;
    }
    return 1;
}


int save_layout_to_path(const char *path, VisNode *layout, size_t n)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("Failed to open layout file for writing");
        return 0;
    }

    size_t i;
    for (i = 0; i < n; i++)
        fprintf(f, "%lf %lf\n", layout[i].x, layout[i].y);

    fclose(f);
    return 1;
}

int load_layout_from_path(const char *path, VisNode **layout_ptr, size_t n)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Layout file does not exist: %s\n", path);
        return 0;
    }

    VisNode *layout = malloc(n * sizeof(VisNode));
    if (!layout) {
        fclose(f);
        fprintf(stderr, "Failed to allocate memory for layout\n");
        return 0;
    }

    size_t i;
    for (i = 0; i < n; i++) {
        if (fscanf(f, "%lf %lf", &layout[i].x, &layout[i].y) != 2) {
            fprintf(stderr, "Layout file corrupted or incomplete\n");
            free(layout);
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    *layout_ptr = layout;
    return 1;
}

// --------- Drawing Functions -----------

static void draw_quadratic_curve(cairo_t *cr, double x0, double y0, double x2, double y2)
{
    double mx = (x0 + x2) / 2;
    double my = (y0 + y2) / 2;
    double dx = x2 - x0;
    double dy = y2 - y0;
    double len = hypot(dx, dy);

    if (len == 0)
        return; // avoid divide by zero

    double offset = len * 0.1;
    double nx = -dy / len;
    double ny = dx / len;
    mx += nx * offset;
    my += ny * offset;

    cairo_move_to(cr, x0, y0);
    cairo_curve_to(cr, mx, my, mx, my, x2, y2);
    cairo_stroke(cr);
}


void save_graph_as_image(graph *g, RGBColor *colors, const char *filename, char **labels,
                         VisNode *layout)
{
    size_t n = g->num_nodes;

    if (!layout) {
        // Allocate layout array and assign random positions
        layout = malloc(n * sizeof(VisNode));
        if (!layout) {
            fprintf(stderr, "Failed to allocate memory for layout\n");
            return;
        }
        if (!assign_random_layout(layout, (int)n)) {
            fprintf(stderr, "Failed to assign random layout\n");
            free(layout);
            return;
        }
    }

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t *cr = cairo_create(surface);

    // Background white
    cairo_set_source_rgb(cr, 0.98, 0.98, 0.98);
    cairo_paint(cr);

    // Draw edges (undirected assumed)
    cairo_set_line_width(cr, 3);
    size_t i, j;
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            int edge_idx = i * (int)n + (int)j;
            if (g->edge_weights[edge_idx] != 0) {
                float color_val = 1.0f - 4*g->edge_weights[edge_idx]; // 1 means white (no edge), 0 means black (full edge)
                cairo_set_source_rgb(cr, color_val, color_val, color_val);
                draw_quadratic_curve(cr, layout[i].x, layout[i].y, layout[j].x, layout[j].y);
            }
        }
    }

    // Draw nodes and labels
    for (i = 0; i < n; i++) {
        float r, g_col, b;

        if (colors && colors[i].r <= 255 && colors[i].g <= 255 && colors[i].b <= 255) {
            r = colors[i].r / 255.0f;
            g_col = colors[i].g / 255.0f;
            b = colors[i].b / 255.0f;
        } else {
            // Default gray color
            r = g_col = b = 128.0f / 255.0f;
        }

        cairo_set_source_rgb(cr, r, g_col, b);
        cairo_arc(cr, layout[i].x, layout[i].y, NODE_RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);

        if (labels && labels[i]) {
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 18);
            cairo_text_extents_t extents;
cairo_text_extents(cr, labels[i], &extents);
double text_x = layout[i].x - extents.width / 2 - extents.x_bearing;
double text_y = layout[i].y - extents.height / 2 - extents.y_bearing;

cairo_move_to(cr, text_x, text_y);
cairo_show_text(cr, labels[i]);
        }
    }

    cairo_destroy(cr);
    cairo_surface_write_to_png(surface, filename);
    cairo_surface_destroy(surface);

    if (!layout) free(layout);  // free only if allocated here
}
/*
static void normalize_and_center_layout(VisNode *layout, size_t n, float width, float height, float margin) {
    if (n == 0) return;

    float min_x = layout[0].x, max_x = layout[0].x;
    float min_y = layout[0].y, max_y = layout[0].y;

    for (size_t i = 1; i < n; i++) {
        if (layout[i].x < min_x) min_x = layout[i].x;
        if (layout[i].x > max_x) max_x = layout[i].x;
        if (layout[i].y < min_y) min_y = layout[i].y;
        if (layout[i].y > max_y) max_y = layout[i].y;
    }

    float range_x = max_x - min_x;
    float range_y = max_y - min_y;
    if (range_x < 1e-6f) range_x = 1.0f;
    if (range_y < 1e-6f) range_y = 1.0f;

    float scale_x = (width * (1 - 2*margin)) / range_x;
    float scale_y = (height * (1 - 2*margin)) / range_y;
    float scale = scale_x < scale_y ? scale_x : scale_y;

    float offset_x = margin * width - min_x * scale + (width - (range_x * scale)) / 2;
    float offset_y = margin * height - min_y * scale + (height - (range_y * scale)) / 2;

    for (size_t i = 0; i < n; i++) {
        layout[i].x = layout[i].x * scale + offset_x;
        layout[i].y = layout[i].y * scale + offset_y;
    }
}


static void clamp_layout_to_canvas(VisNode *layout, size_t n, float width, float height, float node_radius) {
    for (size_t i = 0; i < n; i++) {
        layout[i].x = clamp(layout[i].x, node_radius, width - node_radius);
        layout[i].y = clamp(layout[i].y, node_radius, height - node_radius);
    }
}


void spring_save_graph_as_image(graph *g, RGBColor *colors, const char *filename, 
                               char **labels, VisNode *prev_layout)
{
    size_t n = g->num_nodes;
    VisNode* layout = NULL;

    // Compute layout using spring layout, starting from prev_layout if available
    layout = compute_spring_layout(g, prev_layout, 100, 1920, 1080);
    normalize_and_center_layout(layout, g->num_nodes, 1920, 1080, 0.05f);
    clamp_layout_to_canvas(layout, n, 1920, 1080, 18);
    if (!layout) {
        fprintf(stderr, "Failed to compute spring layout\n");
        return;
    }

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t *cr = cairo_create(surface);

    // Background white
    cairo_set_source_rgb(cr, 0.98, 0.98, 0.98);
    cairo_paint(cr);

    // Draw edges (undirected assumed)
    cairo_set_line_width(cr, 2);
    cairo_set_source_rgb(cr, 0, 0, 0);

    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            int edge_idx = i * (int)n + (int)j;
            if (g->edges[edge_idx] != 0) {
                draw_quadratic_curve(cr, layout[i].x, layout[i].y, layout[j].x, layout[j].y);
            }
        }
    }

    // Draw nodes and labels
    for (size_t i = 0; i < n; i++) {
        float r, g_col, b;

        if (colors && colors[i].r <= 255 && colors[i].g <= 255 && colors[i].b <= 255) {
            r = colors[i].r / 255.0f;
            g_col = colors[i].g / 255.0f;
            b = colors[i].b / 255.0f;
        } else {
            r = g_col = b = 128.0f / 255.0f;
        }

        cairo_set_source_rgb(cr, r, g_col, b);
        cairo_arc(cr, layout[i].x, layout[i].y, NODE_RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);

        if (labels && labels[i]) {
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 18);
            cairo_text_extents_t extents;
            cairo_text_extents(cr, labels[i], &extents);
            double text_x = layout[i].x - extents.width / 2 - extents.x_bearing;
            double text_y = layout[i].y - extents.height / 2 - extents.y_bearing;

            cairo_move_to(cr, text_x, text_y);
            cairo_show_text(cr, labels[i]);
        }
    }

    cairo_destroy(cr);
    cairo_surface_write_to_png(surface, filename);
    cairo_surface_destroy(surface);

    free(layout);
}
*/