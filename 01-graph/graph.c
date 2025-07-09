#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include<math.h>
#define INF 1e9f
#define DIST(i,j) dist[(i)*n + (j)]

float *compute_all_pairs_distances(graph *g)
{
	int n = g->num_nodes;
	float *dist = malloc(n * n * sizeof(float));
	if (!dist)
		return NULL;

#define DIST(i,j) dist[(i)*n + (j)]

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (i == j) {
				DIST(i, j) = 0.0f;
			} else if (g->edges[i * n + j] == 1) {
				DIST(i, j) = g->edge_weights[i * n + j];
			} else {
				DIST(i, j) = INF;
			}
		}
	}

	for (int k = 0; k < n; k++) {
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				if (DIST(i, k) < INF && DIST(k, j) < INF) {
					float alt =
					    DIST(i, k) + DIST(k, j);
					if (alt < DIST(i, j)) {
						DIST(i, j) = alt;
					}
				}
			}
		}
	}

	return dist;
}

graph *create_graph(int n, int is_directed)
{
	graph *g = malloc(sizeof(graph));
	if (!g)
		return NULL;

	g->num_nodes = n;
	g->is_directed = is_directed;
	g->edges = malloc(n * n * sizeof(int));
	g->edge_weights = malloc(n * n * sizeof(float));
	if (!g->edges || !g->edge_weights) {
		free(g->edges);
		free(g->edge_weights);
		free(g);
		return NULL;
	}
	memset(g->edges, 0, n * n * sizeof(int));
	memset(g->edge_weights, 0, n * n * sizeof(float));
	return g;
}

void free_graph(graph *g)
{
	if (!g)
		return;
	free(g->edges);
	free(g->edge_weights);
	free(g);
}

void add_edge(graph *g, int u, int v, float weight)
{
	if (u < 0 || v < 0 || u >= g->num_nodes || v >= g->num_nodes)
		return;

	g->edges[g->num_nodes * u + v] = 1;
	g->edge_weights[g->num_nodes * u + v] = weight;
	if (!g->is_directed) {
		g->edges[g->num_nodes * v + u] = 1;
		g->edge_weights[g->num_nodes * v + u] = weight;
	}
}

int is_connected(graph *g, int u, int v)
{
	return g->edges[g->num_nodes * u + v];
}

float get_weight(graph *g, int u, int v)
{
	return g->edge_weights[g->num_nodes * u + v];
}

graph *read_graph(char *filename)
{
	FILE *file = fopen(filename, "r");
	if (!file) {
		perror("Failed to open file");
		return NULL;
	}
	int size, is_directed = 0;
	fscanf(file, "%d,%d\n", &size, &is_directed);
	graph *g = create_graph(size, is_directed);
	int u, v;
	float weight;
	while (fscanf(file, " (%d,%d,%f) ", &u, &v, &weight) == 3) {
		//printf("Read tuple: (%d, %d, %f)\n", u, v, weight);
		add_edge(g, u, v, weight);
	}
	fclose(file);
	return g;
}

void save_graph(graph *g, char *filename)
{
	FILE *file = fopen(filename, "w");
	if (file == NULL) {
		perror("Error");
		return;
	}
	fprintf(file, "%d, %d\n", g->num_nodes, g->is_directed);
	for (int i = 0; i < g->num_nodes; i++) {
		for (int j = 0; j < g->num_nodes; j++) {
			if (g->edges[g->num_nodes * i + j] == 1) {
				fprintf(file, "(%d,%d,%f)\n", i, j,
					g->edge_weights[g->num_nodes * i +
							j]);
			}
		}
	}
	fclose(file);
}

int *bfs_cluster(int start, int n, float *distances, float *opinions,
		 int *visited, float dist_thresh, float op_thresh)
{
	int *cluster = malloc(n * sizeof(int));
	int front = 0, back = 0;
	cluster[back++] = start;
	visited[start] = 1;

	while (front < back) {
		int u = cluster[front++];
		for (int v = 0; v < n; v++) {
			if (visited[v])
				continue;
			float d = distances[u * n + v];
			float o_diff = fabsf(opinions[u] - opinions[v]);
			if (d < dist_thresh && o_diff < op_thresh) {
				visited[v] = 1;
				cluster[back++] = v;
			}
		}
	}

	// Resize and return
	cluster[back] = -1;	// sentinel
	return cluster;
}

int count_opinion_clusters(float *distances, float *opinions, int n,
			   float dist_thresh, float op_thresh)
{
	int *visited = calloc(n, sizeof(int));
	int num_clusters = 0;

	for (int i = 0; i < n; i++) {
		if (!visited[i]) {
			int *cluster =
			    bfs_cluster(i, n, distances, opinions, visited,
					dist_thresh, op_thresh);
			free(cluster);
			num_clusters++;
		}
	}

	free(visited);
	return num_clusters;
}
