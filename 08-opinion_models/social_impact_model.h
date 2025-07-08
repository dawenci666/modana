#include "../05-abstract_opinion_model/abstract_opinion_model.h"
#include "../06-real_opinion_space_[-1,1]/real_opinion_space_[-1,1].h"
#include <stdlib.h>
#include <math.h>

void free_params(opinion_model* sim);
void social_impact_async_mult_update(opinion_model* model);
opinion_model* create_si_async_mult_model(graph* topology,
    float alpha, float beta);
opinion_model* create_si_async_temporal(graph* topology,
    float alpha, float beta);