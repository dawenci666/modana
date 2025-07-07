#include<stdlib.h>
#include<time.h>
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



int main(void){
	srand(time(NULL));
	srand(time(NULL));
	graph* g2 = generate_small_world(100,2,0.2,0);
	opinion_model* sim = create_si_async_mult_model(g2,2,1,NULL);
	if (sim==NULL){printf("sd");} 
	free_model(sim);
	free_graph(g2);
	//run_simulation(sim,1000,0.001);
	return 0; 
}