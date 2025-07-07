#include "0draw_graph.h"


void save_image_as_graph_wopinion_labels_and_colors(char* graph_filename,
                                                    char* opinion_filename,
                                                    char* imgfilename,
                                                    char* layout_file,
                                                    int num_nodes)
{
    graph* g = read_graph(graph_filename);
    if (!g) {
        fprintf(stderr, "Failed to read graph from %s\n", graph_filename);
        return;
    }

    // Read opinions
    float* opinions = read_opinions(opinion_filename, num_nodes);
    if (!opinions) {
        fprintf(stderr, "Failed to read opinions from %s\n", opinion_filename);
        free_graph(g);
        return;
    }

    // Create colors from opinions: Red (-1) → Green (0) → Blue (+1)
    RGBColor* colors = malloc(sizeof(RGBColor) * num_nodes);
    for (int i = 0; i < num_nodes; i++) {
        float val = opinions[i];
        int r = 0, g_col = 0, b = 0;

        if (val <= 0) {
            float t = val + 1.0f; // [-1, 0] → [0, 1]
            r = (int)(255 * (1 - t));  // Red 255 → 0
            g_col = (int)(255 * t);   // Green 0 → 255
            b = 0;
        } else {
            float t = val; // [0, 1]
            r = 0;
            g_col = (int)(255 * (1 - t)); // Green 255 → 0
            b = (int)(255 * t);          // Blue 0 → 255
        }

        colors[i] = (RGBColor){ r, g_col, b };
    }

    // Create labels with node index and opinion
    char** labels = malloc(sizeof(char*) * num_nodes);
    for (int i = 0; i < num_nodes; i++) {
        labels[i] = malloc(32);
        snprintf(labels[i], 32, "%d\n%.3f", i, opinions[i]);
    }

    // Load or assign layout
    VisNode* layout = NULL;
    if (!load_layout_from_path(layout_file, &layout, num_nodes)) {
        layout = malloc(sizeof(VisNode) * num_nodes);
        if (!assign_random_layout(layout, num_nodes)) {
            fprintf(stderr, "Failed to assign layout\n");
            goto cleanup;
        }
        save_layout_to_path(layout_file, layout, num_nodes);
    }

    // Save graph image
    save_graph_as_image(g, colors, imgfilename, labels, layout);

cleanup:
    free_graph(g);
    free(opinions);
    for (int i = 0; i < num_nodes; i++) free(labels[i]);
    free(labels);
    free(colors);
    free_layout(&layout);
}
