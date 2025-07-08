#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef struct {
    int num_nodes;
    int* edges;        // adjacency matrix (n x n)
    int is_directed;
    float* edge_weights;
    float* opinions;   // opinion per node
} graph;

// Map opinion [0..1] to color blue (#0000ff) to red (#ff0000)
void opinion_to_color(float val, char* out) {
    int r = (int)(255 * val);
    int g = 0;
    int b = 255 - r;
    sprintf(out, "#%02x%02x%02x", r, g, b);
}

// Map edge weight [0..1] to green (#00ff00) to black (#000000)
void weight_to_color(float val, char* out) {
    int g = (int)(255 * (1.0f - val));
    int r = 0;
    int b = 0;
    sprintf(out, "#%02x%02x%02x", r, g, b);
}

// Calculate node degree
int node_degree(graph* g, int node) {
    int deg = 0;
    for (int j = 0; j < g->num_nodes; j++) {
        if (g->edges[node * g->num_nodes + j]) deg++;
        if (g->is_directed && g->edges[j * g->num_nodes + node]) deg++;
    }
    return deg;
}

// Write graph frame to DOT with colors, sizes, labels, edge colors
void graph_to_dot_frame(graph* g, FILE* out) {
    fprintf(out, "%s G {\n", g->is_directed ? "digraph" : "graph");

    fprintf(out, "    splines=true;\n");
    fprintf(out, "    overlap=false;\n");
    fprintf(out, "    sep=\"1.0\";\n");
    fprintf(out, "    edge [minlen=2, constraint=false];\n");

    for (int i = 0; i < g->num_nodes; i++) {
        char color[8];
        opinion_to_color(g->opinions[i], color);
        int deg = node_degree(g, i);
        float size = 0.3f + 0.05f * deg; // sizes around 0.3-0.8
        fprintf(out,
            "    %d [label=\"%.2f\", style=filled, fillcolor=\"%s\", width=%.2f, height=%.2f, fixedsize=true, shape=circle];\n",
            i, g->opinions[i], color, size, size);
    }

    for (int i = 0; i < g->num_nodes; i++) {
        for (int j = 0; j < g->num_nodes; j++) {
            int idx = i * g->num_nodes + j;
            if (g->edges[idx]) {
                if (!g->is_directed && j <= i) continue;

                char ecolor[8];
                weight_to_color(g->edge_weights[idx], ecolor);

                if (g->is_directed)
                    fprintf(out, "    %d -> %d", i, j);
                else
                    fprintf(out, "    %d -- %d", i, j);

                fprintf(out, " [weight=%.2f, color=\"%s\", fontcolor=\"%s\", label=\"%.2f\"];\n",
                        g->edge_weights[idx], ecolor, ecolor, g->edge_weights[idx]);
            }
        }
    }

    fprintf(out, "}\n");
}

void generate_opinions(graph* g) {
    for (int i = 0; i < g->num_nodes; i++) {
        g->opinions[i] = (float)rand() / RAND_MAX;
    }
}

graph* create_random_graph(int num_nodes, int is_directed, float edge_prob) {
    graph* g = malloc(sizeof(graph));
    g->num_nodes = num_nodes;
    g->is_directed = is_directed;
    g->edges = calloc(num_nodes * num_nodes, sizeof(int));
    g->edge_weights = calloc(num_nodes * num_nodes, sizeof(float));
    g->opinions = calloc(num_nodes, sizeof(float));

    generate_opinions(g);

    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            if (i != j) {
                float r = (float)rand() / RAND_MAX;
                if (r < edge_prob) {
                    g->edges[i * num_nodes + j] = 1;
                }
            }
        }
    }

    if (!is_directed) {
        for (int i = 0; i < num_nodes; i++) {
            for (int j = i + 1; j < num_nodes; j++) {
                int idx1 = i * num_nodes + j;
                int idx2 = j * num_nodes + i;
                if (g->edges[idx1] || g->edges[idx2]) {
                    g->edges[idx1] = g->edges[idx2] = 1;
                }
            }
        }
    }

    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            int idx = i * num_nodes + j;
            if (g->edges[idx]) {
                float diff = fabsf(g->opinions[i] - g->opinions[j]);
                g->edge_weights[idx] = (diff < 0.01f) ? 0.01f : diff;
            }
        }
    }

    return g;
}

// Update opinions with oscillation and recompute weights
void update_opinions(graph* g, int frame_num) {
    for (int i = 0; i < g->num_nodes; i++) {
        g->opinions[i] = 0.5f + 0.5f * sinf(frame_num * 0.1f + i);
        if (g->opinions[i] < 0) g->opinions[i] = 0;
        if (g->opinions[i] > 1) g->opinions[i] = 1;
    }

    for (int i = 0; i < g->num_nodes; i++) {
        for (int j = 0; j < g->num_nodes; j++) {
            int idx = i * g->num_nodes + j;
            if (g->edges[idx]) {
                float diff = fabsf(g->opinions[i] - g->opinions[j]);
                g->edge_weights[idx] = (diff < 0.01f) ? 0.01f : diff;
            }
        }
    }
}

void free_graph(graph* g) {
    if (!g) return;
    free(g->edges);
    free(g->edge_weights);
    free(g->opinions);
    free(g);
}

int main() {
    srand((unsigned int)time(NULL));

    int num_nodes = 10;
    int is_directed = 0;       // 0 = undirected
    float edge_prob = 0.3f;
    int num_frames = 100;

    graph* g = create_random_graph(num_nodes, is_directed, edge_prob);

    // Create output directory (Linux/Mac)
    system("mkdir -p frames");

    char filename[64];
    for (int frame = 0; frame < num_frames; frame++) {
        update_opinions(g, frame);

        sprintf(filename, "frames/frame_%03d.dot", frame);
        FILE* f = fopen(filename, "w");
        if (!f) {
            perror("Failed to open dot file");
            free_graph(g);
            return 1;
        }

        graph_to_dot_frame(g, f);
        fclose(f);
    }

    free_graph(g);
    printf("Generated %d frames in ./frames/*.dot\n", num_frames);
    printf("Render frames with neato:\n");
    printf("  for f in frames/*.dot; do neato -Tpng \"$f\" -o \"${f%%.dot}.png\" -Gstart=42 -Gmaxiter=500; done\n");
    printf("Then create video with ffmpeg:\n");
    printf("  ffmpeg -framerate 30 -i frames/frame_%%03d.png -c:v libx264 -pix_fmt yuv420p graph_animation.mp4\n");
    return 0;
}
