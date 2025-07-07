#ifndef GRAPH_H
#define GRAPH_H

typedef struct {
    int num_nodes;
    int* edges;  // n x n adjacency matrix in row-major order
    int is_directed;
    float* edge_weights;        // 1 if directed graph, 0 if undirected
} graph;

graph* create_graph(int n, int is_directed);
void free_graph(graph* g);
void add_edge(graph* g, int u, int v,float weight);
int is_connected(graph* g, int u, int v);
float get_weight(graph*g, int u, int v);
void save_graph(graph* g, char* filename);
graph* read_graph(char* filename);
#endif // GRAPH_H
