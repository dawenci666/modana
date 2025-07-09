#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#define WIDTH 1920
#define HEIGHT 1080
#define MARGIN 50
#define NODE_RADIUS_MIN 10.0
#define NODE_RADIUS_MAX 40.0
#define MAX_ITER 500
#define MAX_NODES 30
#define EDGE_PROB 0.08f  // less dense

typedef struct {
    int num_nodes;
    int* edges;           // adjacency matrix (num_nodes * num_nodes)
    float* edge_weights;  // weights same size
} graph;

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    int* node_indices;
    int node_count;
} cluster_t;

typedef struct {
    int u, v;
    float weight;
} edge_t;

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
                float w = 0.5f + (float)rand() / RAND_MAX;
                g->edge_weights[u*n + v] = w;
                g->edge_weights[v*n + u] = w;
            }
        }
    }
    return g;
}

void free_graph(graph* g) {
    if (!g) return;
    free(g->edges);
    free(g->edge_weights);
    free(g);
}

int is_connected(graph* g, int u, int v) {
    return g->edges[u*g->num_nodes + v] != 0;
}

float get_weight(graph* g, int u, int v) {
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
        // Start roughly centered
        positions[2*i] = WIDTH/2 + (frand() - 0.5f)*WIDTH/4;
        positions[2*i+1] = HEIGHT/2 + (frand() - 0.5f)*HEIGHT/4;
    }

    for (int iter = 0; iter < iterations; iter++) {
        float* disp_x = calloc(n, sizeof(float));
        float* disp_y = calloc(n, sizeof(float));

        // Repulsive forces reduced
        for (int i = 0; i < n; i++) {
            for (int j = i+1; j < n; j++) {
                float dx = positions[2*i] - positions[2*j];
                float dy = positions[2*i+1] - positions[2*j+1];
                float dist = sqrtf(dx*dx + dy*dy) + 1e-6f;
                float force = 0.3f * (k*k) / dist;

                float fx = force * (dx / dist);
                float fy = force * (dy / dist);

                disp_x[i] += fx; disp_y[i] += fy;
                disp_x[j] -= fx; disp_y[j] -= fy;
            }
        }

        // Attractive forces
        for (int u = 0; u < n; u++) {
            for (int v = 0; v < n; v++) {
                if (is_connected(g, u, v)) {
                    float w = get_weight(g, u, v);
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

// -------- Distance computation --------
float* compute_distances(float* positions, int n) {
    float* distances = malloc(n*n*sizeof(float));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float dx = positions[2*i] - positions[2*j];
            float dy = positions[2*i+1] - positions[2*j+1];
            distances[i*n + j] = sqrtf(dx*dx + dy*dy);
        }
    }
    return distances;
}

// -------- Cluster detection (BFS) --------

int* bfs_cluster(int start, int n, float* distances, float* opinions, int* visited, float dist_thresh, float op_thresh) {
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
            if (!visited[v]) {
                if (distances[u*n + v] < dist_thresh && fabsf(opinions[u] - opinions[v]) < op_thresh) {
                    visited[v] = 1;
                    cluster_map[v] = start;
                    queue[back++] = v;
                }
            }
        }
    }
    free(queue);
    return cluster_map;
}

// -------- Cluster assignment --------
int* assign_clusters(int n, float* distances, float* opinions, float dist_thresh, float op_thresh, int* out_num_clusters) {
    int* visited = calloc(n, sizeof(int));
    int* cluster_map = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) cluster_map[i] = -1;

    int cluster_id = 0;

    for (int i = 0; i < n; i++) {
        if (!visited[i]) {
            int* partial_map = bfs_cluster(i, n, distances, opinions, visited, dist_thresh, op_thresh);
            for (int j = 0; j < n; j++) {
                if (partial_map[j] != -1) {
                    cluster_map[j] = cluster_id;
                }
            }
            cluster_id++;
            free(partial_map);
        }
    }

    free(visited);
    *out_num_clusters = cluster_id;
    return cluster_map;
}

// -------- Convex hull (Monotone chain algorithm) --------
typedef struct {
    float x, y;
    int idx;
} point_t;

int point_cmp(const void* a, const void* b) {
    point_t* p1 = (point_t*)a;
    point_t* p2 = (point_t*)b;
    if (p1->x < p2->x) return -1;
    if (p1->x > p2->x) return 1;
    if (p1->y < p2->y) return -1;
    if (p1->y > p2->y) return 1;
    return 0;
}

float cross(const point_t O, const point_t A, const point_t B) {
    return (A.x - O.x)*(B.y - O.y) - (A.y - O.y)*(B.x - O.x);
}

int convex_hull(point_t* points, int n, int* hull) {
    qsort(points, n, sizeof(point_t), point_cmp);

    int k = 0;
    // Lower hull
    for (int i = 0; i < n; i++) {
        while (k >= 2 && cross(points[hull[k-2]], points[hull[k-1]], points[i]) <= 0) k--;
        hull[k++] = i;
    }
    // Upper hull
    int t = k+1;
    for (int i = n-2; i >= 0; i--) {
        while (k >= t && cross(points[hull[k-2]], points[hull[k-1]], points[i]) <= 0) k--;
        hull[k++] = i;
    }
    return k - 1;
}

// -------- Map opinion [-1,1] to RGB --------
void opinion_to_rgb(float opinion, float* r, float* g, float* b) {
    if (opinion < 0.0f) {
        *r = 0.3f + 0.7f * (-opinion);
        *g = 0.3f + 0.7f * (opinion + 1.0f);
        *b = 0.3f;
    } else {
        *r = 0.3f * (1.0f - opinion);
        *g = 1.0f;
        *b = 0.3f + 0.7f * opinion;
    }
}

// -------- Draw cluster hull ----------
void draw_cluster_hull(cairo_t* cr, graph* g, float* positions, int n, int* cluster_map, int cluster_id, float* opinions, float padding) {
    int max_points = n * 3;
    point_t* points = malloc(sizeof(point_t) * max_points);
    int count = 0;

    // Add cluster nodes
    for (int i = 0; i < n; i++) {
        if (cluster_map[i] == cluster_id) {
            points[count].x = positions[2*i];
            points[count].y = positions[2*i + 1];
            points[count].idx = count;
            count++;
        }
    }
    // Add edge endpoints for edges fully inside cluster
    for (int u = 0; u < n; u++) {
        for (int v = u+1; v < n; v++) {
            if (is_connected(g, u, v) &&
                cluster_map[u] == cluster_id && cluster_map[v] == cluster_id) {
                points[count].x = positions[2*u];
                points[count].y = positions[2*u + 1];
                points[count].idx = count;
                count++;

                points[count].x = positions[2*v];
                points[count].y = positions[2*v + 1];
                points[count].idx = count;
                count++;
            }
        }
    }

    if (count < 3) {
        free(points);
        return;
    }

    int* hull_indices = malloc(sizeof(int) * count);
    int hull_size = convex_hull(points, count, hull_indices);

    // Average opinion color
    float r = 0, g_ = 0, b = 0;
    int opinion_count = 0;
    for (int i = 0; i < n; i++) {
        if (cluster_map[i] == cluster_id) {
            float rr, gg, bb;
            opinion_to_rgb(opinions[i], &rr, &gg, &bb);
            r += rr; g_ += gg; b += bb;
            opinion_count++;
        }
    }
    if (opinion_count > 0) {
        r /= opinion_count;
        g_ /= opinion_count;
        b /= opinion_count;
    } else {
        r = g_ = b = 0.5f;
    }

    // Compute centroid
    float centroid_x = 0, centroid_y = 0;
    for (int i = 0; i < hull_size; i++) {
        centroid_x += points[hull_indices[i]].x;
        centroid_y += points[hull_indices[i]].y;
    }
    centroid_x /= hull_size;
    centroid_y /= hull_size;

    // Draw expanded hull with padding
    cairo_new_path(cr);
    for (int i = 0; i < hull_size; i++) {
        float px = points[hull_indices[i]].x;
        float py = points[hull_indices[i]].y;

        float dx = px - centroid_x;
        float dy = py - centroid_y;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 1e-6f) {
            dx = dx / len * padding;
            dy = dy / len * padding;
        }

        float x = px + dx;
        float y = py + dy;

        if (i == 0)
            cairo_move_to(cr, x, y);
        else
            cairo_line_to(cr, x, y);
    }
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, r, g_, b, 0.15);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, r, g_, b, 0.4);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    free(points);
    free(hull_indices);
}

// -------- Draw nodes ----------
void draw_node(cairo_t* cr, float x, float y, float opinion) {
    float r, g, b;
    opinion_to_rgb(opinion, &r, &g, &b);

    float radius = NODE_RADIUS_MIN + (opinion + 1.0f)/2.0f * (NODE_RADIUS_MAX - NODE_RADIUS_MIN);

    cairo_set_source_rgb(cr, r, g, b);
    cairo_arc(cr, x, y, radius, 0, 2*M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_arc(cr, x, y, radius, 0, 2*M_PI);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
}

// -------- Draw edges ----------
void draw_edge(cairo_t* cr, float x1, float y1, float x2, float y2) {
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

// -------- Main --------
int main() {
    srand(time(NULL));

    int n = MAX_NODES;

    graph* g = create_random_graph(n);

    float* positions = malloc(n * 2 * sizeof(float));

    // Random opinions [-1,1]
    float* opinions = malloc(n * sizeof(float));
    for (int i = 0; i < n; i++) {
        opinions[i] = 2.0f * frand() - 1.0f;
    }

    float k = 100.0f;
    spring_layout(g, positions, MAX_ITER, k);
    avoid_node_overlap(positions, n, NODE_RADIUS_MAX, 100);
    scale_and_fit_layout(positions, n, WIDTH, HEIGHT, MARGIN);

    // Compute distances and clusters
    float* distances = compute_distances(positions, n);
    int num_clusters;
    float dist_thresh = 300.0f;
    float op_thresh = 0.7f;
    int* cluster_map = assign_clusters(n, distances, opinions, dist_thresh, op_thresh, &num_clusters);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t* cr = cairo_create(surface);

    // White background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Draw cluster hulls
    for (int c = 0; c < num_clusters; c++) {
        draw_cluster_hull(cr, g, positions, n, cluster_map, c, opinions, 30.0f);
    }

    // Draw edges
    for (int u = 0; u < n; u++) {
        for (int v = u+1; v < n; v++) {
            if (is_connected(g, u, v)) {
                draw_edge(cr, positions[2*u], positions[2*u+1], positions[2*v], positions[2*v+1]);
            }
        }
    }

    // Draw nodes
    for (int i = 0; i < n; i++) {
        draw_node(cr, positions[2*i], positions[2*i+1], opinions[i]);
    }

    cairo_surface_write_to_png(surface, "cluster_graph.png");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    free(positions);
    free(opinions);
    free(distances);
    free(cluster_map);
    free_graph(g);

    printf("Graph image saved as cluster_graph.png\n");

    return 0;
}
