#include "real_opinion_space_[-1,1].h"
#include "11-helpers/get_urandom.h"

float rand_uniform_minus1_to_1() {
    return get_urandom(-1.0f, 1.0f);
}

opinion_space* create_opinions_in_real_ball_of_radius_one(size_t num_agents) {
    srand((unsigned int)time(NULL));

    opinion_space* os = create_opinion_space(num_agents, sizeof(float));
    if (!os) return NULL;

    for (size_t i = 0; i < num_agents; i++) {
        float val;
        do {
            val = rand_uniform_minus1_to_1();
        } while (fabsf(val) > 1.0f);

        os->set_opinion(os, (int)i, &val);
    }

    return os;
}
