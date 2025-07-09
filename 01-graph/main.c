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
#define EDGE_PROB 0.08f  // slightly less dense for bigger graph

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

            int cluster_size = 0;
            for (int j = 0; j < n; j++) {
                if (cluster[j] != -1) cluster_size++;
            }
            if (cluster_size < 3) {
                for (int j = 0; j < n; j++) {
                    if (cluster[j] != -1) visited[j] = 0;
                }
                free(cluster);
                continue;
            }

            for (int j = 0; j < n; j++) {
                if (cluster[j] != -1) out_cluster_map[j] = cluster_id;
            }

            cluster_id++;
            free(cluster);
        }
    }
    free(visited);
    return cluster_id;
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

// -------- Node drawing --------
void draw_node(cairo_t* cr, float x, float y, float opinion) {
    // Map opinion [-1..0..1] to color
    float r, g, b;
    if (opinion < 0.0f) {
        // From red (opinion=-1) to green (opinion=0)
        r = 0.3 + 0.7 * (-opinion);
        g = 0.3 + 0.7 * (opinion + 1.0f);
        b = 0.3;
    } else {
        // From green (opinion=0) to blue (opinion=1)
        r = 0.3 * (1.0f - opinion);
        g = 1.0f;
        b = 0.3 + 0.7 * opinion;
    }
    cairo_set_source_rgb(cr, r, g, b);
    cairo_arc(cr, x, y, NODE_RADIUS_MAX * 0.75, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_arc(cr, x, y, NODE_RADIUS_MAX * 0.75, 0, 2 * M_PI);
    cairo_stroke(cr);
}

// -------- Edge drawing --------
void draw_edge(cairo_t* cr, float x1, float y1, float x2, float y2, float weight) {
    cairo_set_line_width(cr, 10.0f * weight);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

// -------- Convex hull --------

typedef struct {
    float x, y;
    int idx;
} point_t;

int cross(point_t o, point_t a, point_t b) {
    return (a.x - o.x)*(b.y - o.y) - (a.y - o.y)*(b.x - o.x);
}

int compare_points(const void* a, const void* b) {
    point_t* p1 = (point_t*)a;
    point_t* p2 = (point_t*)b;
    if (p1->x == p2->x) return (p1->y > p2->y) - (p1->y < p2->y);
    return (p1->x > p2->x) - (p1->x < p2->x);
}

int convex_hull(point_t* points, int n, int* hull_indices) {
    if (n < 3) return 0;

    qsort(points, n, sizeof(point_t), compare_points);

    int k = 0;
    point_t* hull = malloc(2 * n * sizeof(point_t));

    // Lower hull
    for (int i = 0; i < n; i++) {
        while (k >= 2 && cross(hull[k-2], hull[k-1], points[i]) <= 0) k--;
        hull[k++] = points[i];
    }
    // Upper hull
    int t = k + 1;
    for (int i = n-2; i >= 0; i--) {
        while (k >= t && cross(hull[k-2], hull[k-1], points[i]) <= 0) k--;
        hull[k++] = points[i];
    }

    int hull_size = k - 1;
    for (int i = 0; i < hull_size; i++) {
        hull_indices[i] = hull[i].idx;
    }

    free(hull);
    return hull_size;
}

// -------- Smooth hull drawing (Catmull-Rom spline) --------

// Helper to get point with wrapping
point_t get_wrapped_point(point_t* pts, int n, int idx) {
    while (idx < 0) idx += n;
    while (idx >= n) idx -= n;
    return pts[idx];
}

void draw_smooth_hull(cairo_t* cr, point_t* hull_points, int hull_size) {
    if (hull_size < 3) return;

    // Compute control points for Catmull-Rom spline
    // Formula reference: https://www.mvps.org/directx/articles/catmull/

    cairo_move_to(cr, hull_points[0].x, hull_points[0].y);

    for (int i = 0; i < hull_size; i++) {
        point_t p0 = get_wrapped_point(hull_points, hull_size, i-1);
        point_t p1 = get_wrapped_point(hull_points, hull_size, i);
        point_t p2 = get_wrapped_point(hull_points, hull_size, i+1);
        point_t p3 = get_wrapped_point(hull_points, hull_size, i+2);

        // Draw spline between p1 and p2
        // Calculate control points for cubic Bezier
        float cp1x = p1.x + (p2.x - p0.x) / 6.0f;
        float cp1y = p1.y + (p2.y - p0.y) / 6.0f;
        float cp2x = p2.x - (p3.x - p1.x) / 6.0f;
        float cp2y = p2.y - (p3.y - p1.y) / 6.0f;

        cairo_curve_to(cr, cp1x, cp1y, cp2x, cp2y, p2.x, p2.y);
    }

    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_stroke(cr);
}

// -------- Draw cluster hulls with padding --------

void draw_convex_hull(cairo_t* cr, float* positions, int* cluster_map, int cluster_id, int n) {
    // Collect points of this cluster
    int cluster_size = 0;
    for (int i = 0; i < n; i++) {
        if (cluster_map[i] == cluster_id) cluster_size++;
    }
    if (cluster_size < 3) return;

    point_t* points = malloc(cluster_size * sizeof(point_t));
    int idx = 0;
    for (int i = 0; i < n; i++) {
        if (cluster_map[i] == cluster_id) {
            points[idx].x = positions[2*i];
            points[idx].y = positions[2*i+1];
            points[idx].idx = i;
            idx++;
        }
    }

    int* hull_indices = malloc(cluster_size * sizeof(int));
    int hull_size = convex_hull(points, cluster_size, hull_indices);
    if (hull_size < 3) {
        free(points);
        free(hull_indices);
        return;
    }

    // Build hull points array with padding outward for smooth blob
    point_t* hull_points = malloc(hull_size * sizeof(point_t));
    float padding = NODE_RADIUS_MAX * 1.3f;

    // Calculate outward normals for each hull edge and move points outward
    for (int i = 0; i < hull_size; i++) {
        int curr = hull_indices[i];
        int next = hull_indices[(i+1) % hull_size];

        float x1 = points[curr].x;
        float y1 = points[curr].y;
        float x2 = points[next].x;
        float y2 = points[next].y;

        // Edge vector
        float ex = x2 - x1;
        float ey = y2 - y1;
        float length = sqrtf(ex*ex + ey*ey);
        if (length < 1e-6f) length = 1;

        // Normal vector (perp to edge)
        float nx = -ey / length;
        float ny = ex / length;

        // Average normal for the vertex: For smooth expansion, average normals of two edges
        int prev = hull_indices[(i-1 + hull_size) % hull_size];
        float px = points[curr].x - points[prev].x;
        float py = points[curr].y - points[prev].y;
        float plen = sqrtf(px*px + py*py);
        if (plen < 1e-6f) plen = 1;
        float pnx = -py / plen;
        float pny = px / plen;

        // Average normals
        float avg_nx = (nx + pnx) * 0.5f;
        float avg_ny = (ny + pny) * 0.5f;
        float nlen = sqrtf(avg_nx*avg_nx + avg_ny*avg_ny);
        if (nlen < 1e-6f) nlen = 1;
        avg_nx /= nlen;
        avg_ny /= nlen;

        // Move point outward by padding
        hull_points[i].x = points[curr].x + avg_nx * padding;
        hull_points[i].y = points[curr].y + avg_ny * padding;
    }

    // Draw smooth hull blob
    cairo_set_source_rgba(cr, 0.2, 0.6, 0.8, 0.35);
    cairo_set_line_width(cr, 4.0);

    draw_smooth_hull(cr, hull_points, hull_size);

    free(points);
    free(hull_indices);
    free(hull_points);
}

// -------- Main --------

int main() {
    srand(time(NULL));

    int n = MAX_NODES;

    graph* g = create_random_graph(n);
    float* opinions = malloc(n * sizeof(float));
    float* positions = malloc(2 * n * sizeof(float));
    int* cluster_map = calloc(n, sizeof(int));

    for (int i = 0; i < n; i++) {
        opinions[i] = -1.0f + 2.0f * frand();
    }

    float k = sqrtf((WIDTH * HEIGHT) / (float)n);

    spring_layout(g, positions, MAX_ITER, k);
    avoid_node_overlap(positions, n, NODE_RADIUS_MAX, 30);
    scale_and_fit_layout(positions, n, WIDTH, HEIGHT, MARGIN);

    float* distances = compute_distances(positions, n);

    int num_clusters = xcount_opinion_clusters(distances, opinions, n, 400.0f, 1.0f, cluster_map);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    for (int u = 0; u < n; u++) {
        for (int v = u+1; v < n; v++) {
            if (xis_connected(g, u, v)) {
                float w = xget_weight(g, u, v);
                draw_edge(cr, positions[2*u], positions[2*u+1], positions[2*v], positions[2*v+1], w);
            }
        }
    }

    for (int c = 0; c < num_clusters; c++) {
        draw_convex_hull(cr, positions, cluster_map, c, n);
    }

    for (int i = 0; i < n; i++) {
        draw_node(cr, positions[2*i], positions[2*i+1], opinions[i]);
    }

    cairo_surface_write_to_png(surface, "graph_smooth_hull.png");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    free(opinions);
    free(positions);
    free(cluster_map);
    free(distances);
    xfree_graph(g);

    printf("Graph with %d nodes saved as graph_smooth_hull.png\n", n);

    return 0;
}
