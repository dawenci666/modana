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

int cmp_visnode(const void *a, const void *b)
{
	const VisNode *p1 = (const VisNode *)a, *p2 = (const VisNode *)b;
	if (p1->x != p2->x)
		return (p1->x < p2->x) ? -1 : 1;
	return (p1->y < p2->y) ? -1 : 1;
}

double cross(VisNode O, VisNode A, VisNode B)
{
	return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

VisNode *compute_convex_hull(VisNode *points, int n, int *out_size)
{
	if (n < 3) {
		VisNode *hull = malloc(n * sizeof(VisNode));
		for (int i = 0; i < n; i++)
			hull[i] = points[i];
		*out_size = n;
		return hull;
	}

	qsort(points, n, sizeof(VisNode), cmp_visnode);

	VisNode *hull = malloc(sizeof(VisNode) * (2 * n));
	int k = 0;

	// Lower hull
	for (int i = 0; i < n; i++) {
		while (k >= 2
		       && cross(hull[k - 2], hull[k - 1], points[i]) <= 0)
			k--;
		hull[k++] = points[i];
	}

	// Upper hull
	for (int i = n - 2, t = k + 1; i >= 0; i--) {
		while (k >= t
		       && cross(hull[k - 2], hull[k - 1], points[i]) <= 0)
			k--;
		hull[k++] = points[i];
	}

	*out_size = k - 1;
	return hull;
}

void opinion_to_light_rgb(float op, float *r, float *g, float *b)
{
	if (op < -1.0f)
		op = -1.0f;
	if (op > 1.0f)
		op = 1.0f;

	float t = (op + 1.0f) / 2.0f;	// map [-1,1] to [0,1]

	if (t <= 0.5f) {
		// interpolate between light red and light green
		float local_t = t / 0.5f;	// [0..1]

		*r = 1.0f * (1 - local_t) + 0.6f * local_t;	// 1 -> 0.6
		*g = 0.6f * (1 - local_t) + 1.0f * local_t;	// 0.6 -> 1
		*b = 0.6f;	// constant
	} else {
		// interpolate between light green and light blue
		float local_t = (t - 0.5f) / 0.5f;	// [0..1]

		*r = 0.6f;	// constant 0.6
		*g = 1.0f * (1 - local_t) + 0.6f * local_t;	// 1 -> 0.6
		*b = 0.6f * (1 - local_t) + 1.0f * local_t;	// 0.6 -> 1
	}
}

void draw_cluster_hulls(cairo_t *cr, VisNode *layout, float *node_sizes,
			cluster_result *clusters)
{
	if (!clusters || clusters->num_clusters == 0)
		return;

	const int POINTS_PER_NODE = 8;
	const float PADDING = 10.0f;

	for (int c = 0; c < clusters->num_clusters; c++) {
		int cluster_size = clusters->cluster_sizes[c];
		int total_points = cluster_size * POINTS_PER_NODE;

		VisNode *points = malloc(total_points * sizeof(VisNode));
		if (!points)
			continue;

		int idx = 0;
		for (int i = 0; i < cluster_size; i++) {
			int node_idx = clusters->clusters[c][i];
			double cx = layout[node_idx].x;
			double cy = layout[node_idx].y;
			double radius = node_sizes[node_idx] + PADDING;

			for (int p = 0; p < POINTS_PER_NODE; p++) {
				double angle =
				    2.0 * M_PI * p / POINTS_PER_NODE;
				points[idx].x = cx + radius * cos(angle);
				points[idx].y = cy + radius * sin(angle);
				idx++;
			}
		}

		int hull_size = 0;
		VisNode *hull =
		    compute_convex_hull(points, total_points, &hull_size);
		free(points);

		if (hull_size > 2) {
			// Get cluster average opinion and convert to light RGB color
			float avg_op = clusters->avg_opinions[c];
			float r, g, b;
			opinion_to_light_rgb(avg_op, &r, &g, &b);

			cairo_set_source_rgba(cr, r, g, b, 0.3);	// light fill with transparency
			cairo_set_line_width(cr, 3.0);

			cairo_move_to(cr, hull[0].x, hull[0].y);
			for (int i = 1; i < hull_size; i++) {
				cairo_line_to(cr, hull[i].x, hull[i].y);
			}
			cairo_close_path(cr);
			cairo_fill_preserve(cr);

			// Stroke with a stronger opacity border of same color
			cairo_set_source_rgba(cr, r, g, b, 0.8);
			cairo_stroke(cr);
		}

		free(hull);
	}
}

int position_ok(VisNode *nodes, float *node_sizes, float curr_radius,
		int placed_nodes, double x, double y)
{
	for (int i = 0; i < placed_nodes; i++) {
		double dx = nodes[i].x - x;
		double dy = nodes[i].y - y;
		double min_dist = node_sizes[i] + curr_radius;
		if (hypot(dx, dy) < min_dist)
			return 0;
	}
	return 1;
}

int assign_random_layout(VisNode *nodes, float *node_sizes, int n)
{
	for (int i = 0; i < n; i++) {
		int attempts = 0;
		double x, y;
		float radius = node_sizes[i];

		do {
			// Respect margins based on current node's size
			x = get_urandom(radius, WIDTH - radius);
			y = get_urandom(radius, HEIGHT - radius);

			attempts++;
			if (attempts > MAX_PLACEMENT_ATTEMPTS)
				return 0;

		} while (!position_ok(nodes, node_sizes, radius, i, x, y));

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
			fprintf(stderr,
				"Layout file corrupted or incomplete\n");
			free(layout);
			fclose(f);
			return 0;
		}
	}

	fclose(f);
	*layout_ptr = layout;
	return 1;
}

static void draw_quadratic_curve(cairo_t *cr, double x0, double y0,
				 double x2, double y2)
{
	double mx = (x0 + x2) / 2;
	double my = (y0 + y2) / 2;
	double dx = x2 - x0;
	double dy = y2 - y0;
	double len = hypot(dx, dy);

	if (len == 0)
		return;		// avoid divide by zero

	double offset = len * 0.1;
	double nx = -dy / len;
	double ny = dx / len;
	mx += nx * offset;
	my += ny * offset;

	cairo_move_to(cr, x0, y0);
	cairo_curve_to(cr, mx, my, mx, my, x2, y2);
	cairo_stroke(cr);
}

void save_graph_as_image(graph *g, RGBColor *node_colors,
			 RGBColor *edge_colors, float *node_sizes,
			 const char *filename, char **labels,
			 VisNode *layout, cluster_result *clusters)
{
	size_t n = g->num_nodes;
	int layout_allocated = 0;

	if (!layout) {
		layout = malloc(n * sizeof(VisNode));
		if (!layout) {
			fprintf(stderr,
				"Failed to allocate memory for layout\n");
			return;
		}
		if (!assign_random_layout(layout, node_sizes, (int)n)) {
			fprintf(stderr,
				"Failed to assign random layout\n");
			free(layout);
			return;
		}
		layout_allocated = 1;
	}

	cairo_surface_t *surface =
	    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
	cairo_t *cr = cairo_create(surface);

	// Background
	cairo_set_source_rgb(cr, 0.90, 0.90, 0.90);
	cairo_paint(cr);

	//draw clusters
	draw_cluster_hulls(cr, layout, node_sizes, clusters);
	// Draw edges
	for (size_t i = 0; i < n; i++) {
		for (size_t j = i + 1; j < n; j++) {
			int edge_idx = i * (int)n + (int)j;
			if (g->edges[edge_idx] != 0) {
				float r = edge_colors[edge_idx].r / 255.0f;
				float g_col =
				    edge_colors[edge_idx].g / 255.0f;
				float b = edge_colors[edge_idx].b / 255.0f;

				float bond_strength =
				    1.0f - g->edge_weights[edge_idx];
				float edge_width =
				    5.0f + bond_strength * (10.0f - 5.0f);

				cairo_set_line_width(cr, edge_width);
				cairo_set_source_rgb(cr, r, g_col, b);
				draw_quadratic_curve(cr, layout[i].x,
						     layout[i].y,
						     layout[j].x,
						     layout[j].y);
			}
		}
	}

	// Draw nodes
	for (size_t i = 0; i < n; i++) {
		float r = 128.0f / 255.0f, g_col = 128.0f / 255.0f, b =
		    128.0f / 255.0f;
		if (node_colors) {
			r = node_colors[i].r / 255.0f;
			g_col = node_colors[i].g / 255.0f;
			b = node_colors[i].b / 255.0f;
		}

		cairo_set_source_rgb(cr, r, g_col, b);
		cairo_arc(cr, layout[i].x, layout[i].y, node_sizes[i], 0,
			  2 * M_PI);
		cairo_fill(cr);

		if (labels && labels[i]) {
			cairo_set_source_rgb(cr, 1, 1, 1);	// label text color
			cairo_select_font_face(cr, "Sans",
					       CAIRO_FONT_SLANT_NORMAL,
					       CAIRO_FONT_WEIGHT_BOLD);
			cairo_set_font_size(cr, 18);
			cairo_text_extents_t extents;
			cairo_text_extents(cr, labels[i], &extents);

			double text_x =
			    layout[i].x - extents.width / 2 -
			    extents.x_bearing;
			double text_y =
			    layout[i].y - extents.height / 2 -
			    extents.y_bearing;

			cairo_move_to(cr, text_x, text_y);
			cairo_show_text(cr, labels[i]);
		}
	}

	cairo_destroy(cr);
	cairo_surface_write_to_png(surface, filename);
	cairo_surface_destroy(surface);

	if (layout_allocated)
		free(layout);
}
