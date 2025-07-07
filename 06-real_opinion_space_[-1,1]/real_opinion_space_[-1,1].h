#ifndef REAL_OPINION_SPACE_H
#define REAL_OPINION_SPACE_H

#include "../04-abstract_opinion_space/abstract_opinion_space.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
// Creates an opinion_space of floats ∈ [-1, 1], uniformly sampled
opinion_space* create_opinions_in_real_ball_of_radius_one(size_t num_agents);
#endif // REAL_OPINION_SPACE_H
