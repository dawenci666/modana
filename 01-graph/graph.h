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
float* compute_all_pairs_distances(graph* g);
graph* read_graph(char* filename);
int* bfs_cluster(int start, int n, float* distances, float* opinions, int* visited, float dist_thresh, float op_thresh);
int count_opinion_clusters(float* distances, float* opinions, int n, float dist_thresh, float op_thresh);

#endif // GRAPH_H
