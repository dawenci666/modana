#include "draw_graph_opinion_labels.h"

char **build_opinion_labels(const float *opinions, size_t num_nodes) {
    char **labels = malloc(sizeof(char *) * num_nodes);
    if (!labels)
        return NULL;

    for (size_t i = 0; i < num_nodes; i++) {
        labels[i] = malloc(MAX_LABEL_LEN);
        if (!labels[i]) {
            for (size_t j = 0; j < i; j++)
                free(labels[j]);
            free(labels);
            return NULL;
        }
        snprintf(labels[i], MAX_LABEL_LEN, "%zu - %.3f", i, opinions[i]);
    }
    return labels;
}

void free_labels(char **labels, size_t num_labels) {
    if (!labels)
        return;
    for (size_t i = 0; i < num_labels; i++) {
        free(labels[i]);
    }
    free(labels);
}

float *read_opinions(const char *filename, size_t *out_num_nodes) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open opinions file");
        return NULL;
    }

    size_t count = 0;
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n')
            count++;
    }
    rewind(file);

    float *opinions = calloc(count, sizeof(float));
    if (!opinions) {
        fclose(file);
        return NULL;
    }

    size_t idx;
    float val;
    while (fscanf(file, "%zu %f\n", &idx, &val) == 2) {
        if (idx >= count) {
            fprintf(stderr, "Warning: node index %zu out of range in opinions file\n", idx);
            continue;
        }
        opinions[idx] = val;
    }
    fclose(file);

    *out_num_nodes = count;
    return opinions;
}

char *path_layout_file(const char *directory) {
    size_t len = strlen(directory) + strlen("/graph.layout") + 1;
    char *buf = malloc(len);
    if (!buf) return NULL;
    snprintf(buf, len, "%s/graph.layout", directory);
    return buf;
}

void save_image_as_graph_wopinion_labels_and_colors(char *graph_filename,
                                                    char *opinion_filename,
                                                    char *imgfilename,
                                                    char *layout_file,
                                                    int num_nodes_hint)
{
    graph *g = read_graph(graph_filename);
    if (!g) {
        fprintf(stderr, "Failed to read graph from %s\n", graph_filename);
        return;
    }

    size_t num_nodes = 0;
    float *opinions = read_opinions(opinion_filename, &num_nodes);
    if (!opinions) {
        fprintf(stderr, "Failed to read opinions from %s\n", opinion_filename);
        free_graph(g);
        return;
    }

    RGBColor *colors = malloc(sizeof(RGBColor) * num_nodes);
    for (size_t i = 0; i < num_nodes; i++) {
        float val = opinions[i];
        int r = 0, g_col = 0, b = 0;

        if (val <= 0) {
            float t = val + 1.0f;
            r = (int)(255 * (1 - t));
            g_col = (int)(255 * t);
        } else {
            float t = val;
            g_col = (int)(255 * (1 - t));
            b = (int)(255 * t);
        }

        colors[i] = (RGBColor){ r, g_col, b };
    }

    char **labels = build_opinion_labels(opinions, num_nodes);
    if (!labels) {
        fprintf(stderr, "Failed to build labels for %zu nodes\n", num_nodes);
        goto cleanup;
    }

    VisNode *layout = NULL;
    if (!load_layout_from_path(layout_file, &layout, num_nodes)) {
        layout = malloc(sizeof(VisNode) * num_nodes);
        if (!assign_random_layout(layout, num_nodes)) {
            fprintf(stderr, "Failed to assign layout\n");
            goto cleanup;
        }
        save_layout_to_path(layout_file, layout, num_nodes);
    }

    save_graph_as_image(g, colors, imgfilename, labels, layout);

cleanup:
    free_graph(g);
    free(opinions);
    free_labels(labels, num_nodes);
    free(colors);
    free_layout(&layout);
}
