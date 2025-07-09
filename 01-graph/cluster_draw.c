#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#define WIDTH 1920
#define HEIGHT 1080
#define MARGIN 50
#define NODE_RADIUS 8.0
#define MAX_ITER 500
#define MAX_NODES 50
#define EDGE_PROB 0.1f

typedef struct {
    int num_nodes;
    int* edges;       // adjacency matrix (num_nodes * num_nodes)
    float* edge_weights; // weights same size
} graph;

// --- Graph functions ---
graph* create_random_graph(int n) {
    graph* g = malloc(sizeof(graph));
    g->num_nodes = n;
    g->edges = calloc(n*n, sizeof(int));
    g->edge_weights = calloc(n*n, sizeof(float));

    for (int u = 0; u < n; u++) {
        for (int v = u+1; v < n; v++) {
            if ((float)rand() / RAND_MAX < EDGE_PROB) {
                g->edges[u*n + v] = 1;
                g->edges[v*n + u] = 1;
                // Random weight [0.5,1.5]
                float w = 0.5f + (float)rand() / RAND_MAX;
                g->edge_weights[u*n + v] = w;
                g->edge_weights[v*n + u] = w;
            }
        }
    }
    return g;
}

void xfree_graph(graph* g) {
    if (!g) return;
    free(g->edges);
    free(g->edge_weights);
    free(g);
}

int xis_connected(graph* g, int u, int v) {
    return g->edges[u*g->num_nodes + v] != 0;
}

float xget_weight(graph* g, int u, int v) {
    return g->edge_weights[u*g->num_nodes + v];
}

// Random float [0,1]
float frand() {
    return (float)rand() / RAND_MAX;
}

// --------- Spring Layout -------------
void spring_layout(graph* g, float* positions, int iterations, float k) {
    int n = g->num_nodes;

    for (int i = 0; i < n; i++) {
        positions[2*i] = frand() * WIDTH;
        positions[2*i+1] = frand() * HEIGHT;
    }

    for (int iter = 0; iter < iterations; iter++) {
        float* disp_x = calloc(n, sizeof(float));
        float* disp_y = calloc(n, sizeof(float));

        // Repulsive forces
        for (int i = 0; i < n; i++) {
            for (int j = i+1; j < n; j++) {
                float dx = positions[2*i] - positions[2*j];
                float dy = positions[2*i+1] - positions[2*j+1];
                float dist = sqrtf(dx*dx + dy*dy) + 1e-6f;
                float force = (k*k) / dist;

                float fx = force * (dx / dist);
                float fy = force * (dy / dist);

                disp_x[i] += fx; disp_y[i] += fy;
                disp_x[j] -= fx; disp_y[j] -= fy;
            }
        }

        // Attractive forces based on adjacency matrix
        for (int u = 0; u < n; u++) {
            for (int v = 0; v < n; v++) {
                if (xis_connected(g, u, v)) {
                    float w = xget_weight(g, u, v);
                    float dx = positions[2*v] - positions[2*u];
                    float dy = positions[2*v+1] - positions[2*u+1];
                    float dist = sqrtf(dx*dx + dy*dy) + 1e-6f;
                    float force = (dist*dist) / k * w;

                    float fx = force * (dx / dist);
                    float fy = force * (dy / dist);

                    disp_x[u] += fx; disp_y[u] += fy;
                    disp_x[v] -= fx; disp_y[v] -= fy;
                }
            }
        }

        // Update positions
        float max_disp = 10.0f;
        for (int i = 0; i < n; i++) {
            float dx = disp_x[i];
            float dy = disp_y[i];
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist > max_disp) {
                dx = dx / dist * max_disp;
                dy = dy / dist * max_disp;
            }
            positions[2*i] += dx;
            positions[2*i+1] += dy;

            // Keep inside canvas
            if (positions[2*i] < 0) positions[2*i] = 0;
            if (positions[2*i] > WIDTH) positions[2*i] = WIDTH;
            if (positions[2*i+1] < 0) positions[2*i+1] = 0;
            if (positions[2*i+1] > HEIGHT) positions[2*i+1] = HEIGHT;
        }

        free(disp_x);
        free(disp_y);
    }
}

// -------- Avoid overlap ----------
void avoid_node_overlap(float* positions, int n, float node_radius, int iterations) {
    for (int iter = 0; iter < iterations; iter++) {
        bool moved = false;
        for (int i = 0; i < n; i++) {
            for (int j = i+1; j < n; j++) {
                float dx = positions[2*j] - positions[2*i];
                float dy = positions[2*j+1] - positions[2*i+1];
                float dist = sqrtf(dx*dx + dy*dy);
                float min_dist = 2.0f * node_radius;

                if (dist < min_dist && dist > 1e-6f) {
                    float overlap = min_dist - dist;
                    float shift_x = (overlap / 2) * (dx / dist);
                    float shift_y = (overlap / 2) * (dy / dist);

                    positions[2*i] -= shift_x;
                    positions[2*i+1] -= shift_y;
                    positions[2*j] += shift_x;
                    positions[2*j+1] += shift_y;
                    moved = true;
                }
            }
        }
        if (!moved) break;
    }
}

// -------- Scale and fit ---------
void scale_and_fit_layout(float* positions, int n, float width, float height, float margin) {
    float min_x = positions[0], max_x = positions[0];
    float min_y = positions[1], max_y = positions[1];

    for (int i = 1; i < n; i++) {
        if (positions[2*i] < min_x) min_x = positions[2*i];
        if (positions[2*i] > max_x) max_x = positions[2*i];
        if (positions[2*i+1] < min_y) min_y = positions[2*i+1];
        if (positions[2*i+1] > max_y) max_y = positions[2*i+1];
    }

    float size_x = max_x - min_x;
    float size_y = max_y - min_y;

    float scale_x = (width - 2*margin) / (size_x > 0 ? size_x : 1);
    float scale_y = (height - 2*margin) / (size_y > 0 ? size_y : 1);
    float scale = scale_x < scale_y ? scale_x : scale_y;

    for (int i = 0; i < n; i++) {
        positions[2*i] = (positions[2*i] - min_x) * scale + margin;
        positions[2*i+1] = (positions[2*i+1] - min_y) * scale + margin;
    }
}

// -------- Cluster detection (BFS) --------

int* xbfs_cluster(int start, int n, float* distances, float* opinions, int* visited, float dist_thresh, float op_thresh) {
    int* cluster_map = calloc(n, sizeof(int));
    for (int i = 0; i < n; i++) cluster_map[i] = -1;

    int* queue = malloc(n * sizeof(int));
    int front = 0, back = 0;

    queue[back++] = start;
    visited[start] = 1;
    cluster_map[start] = start;

    while (front < back) {
        int u = queue[front++];
        for (int v = 0; v < n; v++) {
            if (!visited[v] && distances[u*n+v] < dist_thresh && fabsf(opinions[u] - opinions[v]) < op_thresh) {
                visited[v] = 1;
                cluster_map[v] = start;
                queue[back++] = v;
            }
        }
    }

    free(queue);
    return cluster_map;
}

int xcount_opinion_clusters(float* distances, float* opinions, int n, float dist_thresh, float op_thresh, int* out_cluster_map) {
    int* visited = calloc(n, sizeof(int));
    int cluster_id = 0;

    for (int i = 0; i < n; i++) {
        if (!visited[i]) {
            int* cluster = xbfs_cluster(i, n, distances, opinions, visited, dist_thresh, op_thresh);
            for (int j = 0; j < n; j++) {
                if (cluster[j] != -1) {
                    out_cluster_map[j] = cluster_id;
                }
            }
            cluster_id++;
            free(cluster);
        }
    }

    free(visited);
    return cluster_id;
}

float* compute_distances(float* positions, int n) {
    float* distances = malloc(n * n * sizeof(float));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float dx = positions[2*i] - positions[2*j];
            float dy = positions[2*i+1] - positions[2*j+1];
            distances[i*n + j] = sqrtf(dx*dx + dy*dy);
        }
    }
    return distances;
}

// --- Drawing helpers ---

void draw_node(cairo_t* cr, float x, float y, float opinion) {
    double r = opinion > 0 ? 1.0 : 0.3;
    double b = opinion < 0 ? 1.0 : 0.3;
    double g = 0.3;

    cairo_set_source_rgb(cr, r, g, b);
    cairo_arc(cr, x, y, NODE_RADIUS, 0, 2*M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
}

void draw_edge(cairo_t* cr, float x0, float y0, float x1, float y1) {
    float ctrl_x = (x0 + x1)/2 + (y1 - y0)/8;
    float ctrl_y = (y0 + y1)/2 + (x0 - x1)/8;

    cairo_move_to(cr, x0, y0);
    cairo_curve_to(cr, ctrl_x, ctrl_y, ctrl_x, ctrl_y, x1, y1);
    cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.5);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
}

// Convex hull helpers

typedef struct { float x, y; } Point;

float cross(Point O, Point A, Point B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

int compare_points(const void* a, const void* b) {
    Point* p1 = (Point*)a;
    Point* p2 = (Point*)b;
    if (p1->x < p2->x) return -1;
    if (p1->x > p2->x) return 1;
    if (p1->y < p2->y) return -1;
    if (p1->y > p2->y) return 1;
    return 0;
}

int convex_hull(Point* points, int n, Point* hull) {
    int k = 0;
    qsort(points, n, sizeof(Point), compare_points);

    for (int i = 0; i < n; i++) {
        while (k >= 2 && cross(hull[k-2], hull[k-1], points[i]) <= 0) k--;
        hull[k++] = points[i];
    }
    for (int i = n-2, t = k+1; i >= 0; i--) {
        while (k >= t && cross(hull[k-2], hull[k-1], points[i]) <= 0) k--;
        hull[k++] = points[i];
    }
    return k;
}

void random_mellow_color(double* r, double* g, double* b) {
    *r = 0.6 + 0.4 * frand();
    *g = 0.6 + 0.4 * frand();
    *b = 0.6 + 0.4 * frand();
}

void draw_cluster_hull(cairo_t* cr, float* positions, int* cluster_map, int n, int cluster_id) {
    Point* points = malloc(n * sizeof(Point));
    int count = 0;

    for (int i = 0; i < n; i++) {
        if (cluster_map[i] == cluster_id) {
            points[count].x = positions[2*i];
            points[count].y = positions[2*i+1];
            count++;
        }
    }

    if (count < 3) {
        free(points);
        return;
    }

    Point* hull = malloc(count * sizeof(Point));
    int hull_size = convex_hull(points, count, hull);

    double r, g, b;
    random_mellow_color(&r, &g, &b);

    cairo_set_source_rgba(cr, r, g, b, 0.15);
    cairo_move_to(cr, hull[0].x, hull[0].y);
    for (int i = 1; i < hull_size; i++) {
        cairo_line_to(cr, hull[i].x, hull[i].y);
    }
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, hull[0].x, hull[0].y);
    for (int i = 1; i < hull_size; i++) {
        cairo_line_to(cr, hull[i].x, hull[i].y);
    }
    cairo_close_path(cr);
    cairo_stroke(cr);

    free(points);
    free(hull);
}

// Main render function
void render_graph(graph* g, float* opinions) {
    int n = g->num_nodes;
    float* positions = malloc(2 * n * sizeof(float));

    spring_layout(g, positions, MAX_ITER, 30.0f);
    avoid_node_overlap(positions, n, NODE_RADIUS, 50);
    scale_and_fit_layout(positions, n, WIDTH, HEIGHT, MARGIN);

    float* distances = compute_distances(positions, n);

    int* cluster_map = malloc(n * sizeof(int));
    int num_clusters = xcount_opinion_clusters(distances, opinions, n, 150.0f, 0.5f, cluster_map);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    for (int c = 0; c < num_clusters; c++) {
        draw_cluster_hull(cr, positions, cluster_map, n, c);
    }

    for (int u = 0; u < n; u++) {
        for (int v = u+1; v < n; v++) {
            if (xis_connected(g, u, v)) {
                draw_edge(cr, positions[2*u], positions[2*u+1], positions[2*v], positions[2*v+1]);
            }
        }
    }

    for (int i = 0; i < n; i++) {
        draw_node(cr, positions[2*i], positions[2*i+1], opinions[i]);
    }

    cairo_surface_write_to_png(surface, "graph_render.png");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(positions);
    free(distances);
    free(cluster_map);
}

int main() {
    srand(time(NULL));
    int n = MAX_NODES;

    graph* g = create_random_graph(n);

    float* opinions = malloc(n * sizeof(float));
    for (int i = 0; i < n; i++) {
        opinions[i] = 2.0f * frand() - 1.0f;  // random in [-1,1]
    }

    render_graph(g, opinions);

    free(opinions);
    xfree_graph(g);

    printf("Graph rendered to graph_render.png\n");
    return 0;
}
