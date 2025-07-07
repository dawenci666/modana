#include "draw_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cairo/cairo.h>

// --------- Layout Utilities -----------

void free_layout(VisNode **layout_ptr)
{
    if (*layout_ptr) {
        free(*layout_ptr);
        *layout_ptr = NULL;
    }
}

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
            x = rand() % (WIDTH - 2 * NODE_RADIUS) + NODE_RADIUS;
            y = rand() % (HEIGHT - 2 * NODE_RADIUS) + NODE_RADIUS;
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
    cairo_set_line_width(cr, 2);
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);

    size_t i, j;
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            // Check if edge exists (nonzero)
            int edge_idx = i * (int)n + (int)j;
            if (g->edges[edge_idx] != 0) {
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
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 12);
            cairo_move_to(cr, layout[i].x + NODE_RADIUS + 2, layout[i].y + 4); // Approx vertical centering
            cairo_show_text(cr, labels[i]);
        }
    }

    cairo_destroy(cr);
    cairo_surface_write_to_png(surface, filename);
    cairo_surface_destroy(surface);

    if (!layout) free(layout);  // free only if allocated here
}
