#include "../05-abstract_opinion_model/abstract_opinion_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir_safe(path) _mkdir(path)
#else
#include <sys/stat.h>
#define mkdir_safe(path) mkdir(path, 0755)
#endif

int run_simulation(opinion_model * model, size_t max_steps,
		   float convergence_threshold, const char *directoryname,
		   int save_data);
