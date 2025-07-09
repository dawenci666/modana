#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cairo.h>
#include "graph.h"
#include "../02-graph_topologies/graph_generators.h"

#define ITERATIONS      500
#define COOLING         0.95f
#define EPSILON         0.000001f

// Maximum radius any node can have in draw
#define NODE_RADIUS_MAX 20.0f
// Safety margin = max node radius + wiggle room for curves
#define MARGIN          (NODE_RADIUS_MAX + 10.0f)

typedef struct {
    float x, y;
    int   idx;
} point;

// --- Utility ---
static float rand_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

// --- Layout Algorithm ---
point* compute_layout(graph* g, int width, int height) {
    int   n      = g->num_nodes;
    point* points = malloc(sizeof(point) * n);
    float* disp_x = calloc(n, sizeof(float));
    float* disp_y = calloc(n, sizeof(float));
    if (!points || !disp_x || !disp_y) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    srand((unsigned)time(NULL));
    // init within [MARGIN, width–MARGIN]
    for (int i = 0; i < n; i++) {
        points[i].x   = rand_float(MARGIN,       width  - MARGIN);
        points[i].y   = rand_float(MARGIN,       height - MARGIN);
        points[i].idx = i;
    }

    float k = sqrtf(((float)width * height) / n);
    float t = fminf(width, height) / 10.0f;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        // zero displacements
        for (int i = 0; i < n; i++) disp_x[i] = disp_y[i] = 0.0f;

        // repulsion
        for (int v = 0; v < n; v++) {
            for (int u = v + 1; u < n; u++) {
                float dx   = points[v].x - points[u].x;
                float dy   = points[v].y - points[u].y;
                float dist = sqrtf(dx*dx + dy*dy) + EPSILON;
                float f    = (k*k) / dist;
                float fx   = f * dx / dist;
                float fy   = f * dy / dist;
                disp_x[v] += fx; disp_y[v] += fy;
                disp_x[u] -= fx; disp_y[u] -= fy;
            }
        }

        // attraction
        for (int v = 0; v < n; v++) {
            for (int u = 0; u < n; u++) {
                if (is_connected(g, v, u)) {
                    float dx   = points[v].x - points[u].x;
                    float dy   = points[v].y - points[u].y;
                    float dist = sqrtf(dx*dx + dy*dy) + EPSILON;
                    float w    = get_weight(g, v, u);
                    float ideal = k * (1.0f - w);
                    float f     = w * logf(dist / ideal);
                    float fx    = f * dx / dist;
                    float fy    = f * dy / dist;
                    disp_x[v]  -= fx; disp_y[v]  -= fy;
                    if (!g->is_directed) {
                        disp_x[u] += fx; disp_y[u] += fy;
                    }
                }
            }
        }

        // move + clamp
        for (int i = 0; i < n; i++) {
            float dlen = sqrtf(disp_x[i]*disp_x[i] + disp_y[i]*disp_y[i]);
            if (dlen > EPSILON) {
                float dx = disp_x[i] / dlen * fminf(dlen, t);
                float dy = disp_y[i] / dlen * fminf(dlen, t);
                points[i].x += dx;
                points[i].y += dy;
                // clamp to margins
                if (points[i].x < MARGIN)            points[i].x = MARGIN;
                else if (points[i].x > width - MARGIN)  points[i].x = width  - MARGIN;
                if (points[i].y < MARGIN)            points[i].y = MARGIN;
                else if (points[i].y > height - MARGIN) points[i].y = height - MARGIN;
            }
        }

        // cool
        t *= COOLING;
        if (t < 1.0f) t = 1.0f;
    }

    free(disp_x);
    free(disp_y);
    return points;
}

// --- Drawing ---
static void draw_node(cairo_t* cr, float x, float y, float r) {
    cairo_arc(cr, x, y, r, 0, 2*M_PI);
    cairo_fill(cr);
}

static void draw_quadratic_bezier(cairo_t* cr,
    float x0, float y0, float cx, float cy, float x1, float y1)
{
    cairo_move_to(cr, x0, y0);
    cairo_curve_to(cr,
        (x0 + 2*cx)/3, (y0 + 2*cy)/3,
        (2*cx + x1)/3, (2*cy + y1)/3,
        x1, y1);
    cairo_stroke(cr);
}

void draw_graph_cairo(cairo_t* cr,
                      graph* g,
                      point* layout,
                      float* node_values,
                      int width,
                      int height)
{
    int n = g->num_nodes;

    // --- 1) Compute per-node radii & margins, clamp centers ---
    float* radii  = malloc(sizeof(float) * n);
    float* margin = malloc(sizeof(float) * n);
    const float pad = 10.0f;

    for (int i = 0; i < n; i++) {
        radii[i]      = 5.0f + 15.0f * fabsf(node_values[i]);
        margin[i]     = radii[i] + pad;
        // Re-clamp node centers inside [margin, width-margin] etc.
        if (layout[i].x < margin[i])              layout[i].x = margin[i];
        else if (layout[i].x > width  - margin[i]) layout[i].x = width  - margin[i];
        if (layout[i].y < margin[i])              layout[i].y = margin[i];
        else if (layout[i].y > height - margin[i]) layout[i].y = height - margin[i];
    }

    // --- 2) Paint background ---
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // --- 3) Draw edges (skip duplicates / self‑loops) ---
    for (int u = 0; u < n; u++) {
        int v_start = g->is_directed ? 0 : u + 1;
        for (int v = v_start; v < n; v++) {
            if (u == v)                       continue;
            if (!is_connected(g, u, v))      continue;

            float x0 = layout[u].x, y0 = layout[u].y;
            float x1 = layout[v].x, y1 = layout[v].y;

            // midpoint + perpendicular offset
            float mx = 0.5f*(x0 + x1), my = 0.5f*(y0 + y1);
            float dx = y1 - y0, dy = x0 - x1;
            float dlen = sqrtf(dx*dx + dy*dy) + EPSILON;
            dx /= dlen; dy /= dlen;

            const float curve_offset = 10.0f;
            float cx = mx + dx * curve_offset;
            float cy = my + dy * curve_offset;

            // clamp control point inside both margins
            float mrg = fminf(margin[u], margin[v]);
            cx = fminf(fmaxf(cx, mrg), width  - mrg);
            cy = fminf(fmaxf(cy, mrg), height - mrg);

            // compute edge width
            float w  = get_weight(g, u, v);
            float ew = 0.5f + 4.5f * (1.0f - w);

            cairo_set_line_width(cr, ew);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_move_to(cr, x0, y0);
            cairo_curve_to(cr,
                (x0 + 2*cx)/3, (y0 + 2*cy)/3,
                (2*cx + x1)/3, (2*cy + y1)/3,
                x1, y1
            );
            cairo_stroke(cr);
        }
    }

    // --- 4) Draw nodes on top ---
    for (int i = 0; i < n; i++) {
        float x = layout[i].x, y = layout[i].y, r = radii[i];
        cairo_set_source_rgb(cr, 0.2, 0.4, 0.8);
        cairo_arc(cr, x, y, r, 0, 2*M_PI);
        cairo_fill(cr);

        cairo_set_line_width(cr, 1.0);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_arc(cr, x, y, r, 0, 2*M_PI);
        cairo_stroke(cr);
    }

    free(radii);
    free(margin);
}




// --- main ---
int main(void) {
    int width = 1920, height = 1080;
    graph* g = generate_erdos_renyi(5, 0.6f, 0);
    float node_values[5] = {0.5f,-0.7f,0.3f,-1.0f,0.8f};

    point* layout = compute_layout(g, width, height);

    cairo_surface_t* surf = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surf);

    draw_graph_cairo(cr, g, layout, node_values, width, height);

    cairo_surface_write_to_png(surf, "graph.png");

    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    free(layout);
    free_graph(g);
    printf("Saved to graph.png\n");
    return 0;
}
