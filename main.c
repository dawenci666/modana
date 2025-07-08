#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>   // For mkdir

#include "00-vector/vector.h"
#include "01-graph/graph.h"
#include "02-graph_topologies/graph_generators.h"
#include "03-draw_graph/draw_graph.h"
#include "04-abstract_opinion_space/abstract_opinion_space.h"
#include "05-abstract_opinion_model/abstract_opinion_model.h"
#include "06-real_opinion_space_[-1,1]/real_opinion_space_[-1,1].h"
#include "07-draw_graph_with_opinion_labels/draw_graph_opinion_labels.h"
#include "08-opinion_models/social_impact_model.h"
#include "09-abstract_opinion_model_simulation/abstract_opinion_model_simulation.h"
#include "10_gen_video_from_images/gen_video_from_images.h"
#include "11-helpers/create_dir_with_curr_timestamp.h"
#include "11-helpers/get_urandom.h"

int main(void) {
    srand(time(NULL));
    graph* g2 = generate_erdos_renyi(30, 0.3, 0);

    opinion_model* sim = create_si_async_temporal(g2, 2, 1);
    if (sim == NULL) {
        printf("Failed to create model\n");
        return 1;
    }

    char* outdir = create_dir_with_curr_timestamp("simls_raw_data");
    int step_count = run_simulation(sim, 1000, 0.001, outdir);
    if(step_count<999){
            VisNode* layout = malloc(g2->num_nodes * sizeof(VisNode));
    if (!layout) {
        printf("Failed to allocate layout\n");
        free_model(sim);
        free_graph(g2);
        free(outdir);
        return 1;
    }
    assign_random_layout(layout, g2->num_nodes);
        close_urandom();
    char layout_path[256];
    strcpy(layout_path,outdir);
    strcat(layout_path,"/graph.layout");
    if (!save_layout_to_path(layout_path, layout, g2->num_nodes)) {
        printf("Failed to save layout\n");
        free(layout);
        free_model(sim);
        free_graph(g2);
        free(outdir);
        return 1;
    }

    // Create images directory inside outdir
    char images_dir[512];
    snprintf(images_dir, sizeof(images_dir), "%s/images", outdir);
    mkdir(images_dir, 0755);

    for (int i = 0; i <= step_count; i++) {
        printf("\rGenerating picture %d/%d", i, step_count);
        fflush(stdout);
        char graph_path[512];
        char opinion_path[512];
        char img_path[512];

        snprintf(graph_path, sizeof(graph_path), "%s/%d.graph", outdir, i);
        snprintf(opinion_path, sizeof(opinion_path), "%s/%d.opinions", outdir, i);
        snprintf(img_path, sizeof(img_path), "%s/%04d.png", images_dir, i);

        save_image_as_graph_wopinion_labels_and_colors(
            graph_path,
            opinion_path,
            img_path,
            layout_path,
            g2->num_nodes
        );
    }
    free(layout);
    generate_video_from_images(outdir, NULL, 5);

    }
    free_opinion_space(sim->opinion_space);
    free_params(sim);
    free_model(sim);
    free_graph(g2);
    free(outdir);
    close_urandom();
    return 0;
}
