# Compiler and flags
CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -MMD -MP \
         -I. \
         -I00-vector \
         -I01-graph \
         -I02-graph_topologies \
         -I03-draw_graph \
         -I04-abstract_opinion_space \
         -I05-abstract_opinion_model \
         -I06-real_opinion_space_[-1,1] \
         -I07-draw_graph_with_opinion_labels \
         -I08-opinion_models \
         -I09-abstract_opinion_model_simulation \
         -I10_gen_video_from_images \
         -I11-helpers \
         $(shell pkg-config --cflags cairo)

LDFLAGS = $(shell pkg-config --libs cairo) -lm

# Source files
SRC = \
    main.c \
    00-vector/vector.c \
    01-graph/graph.c \
    02-graph_topologies/graph_generators.c \
    03-draw_graph/draw_graph.c \
    04-abstract_opinion_space/abstract_opinion_space.c \
    05-abstract_opinion_model/abstract_opinion_model.c \
    06-real_opinion_space_[-1,1]/real_opinion_space_[-1,1].c \
    07-draw_graph_with_opinion_labels/draw_graph_opinion_labels.c \
    08-opinion_models/social_impact_model.c \
    09-abstract_opinion_model_simulation/abstract_opinion_model_simulation.c \
    10_gen_video_from_images/gen_video_from_images.c \
    11-helpers/create_dir_with_curr_timestamp.c

# Object and dependency files (with directory structure)
OBJ = $(patsubst %.c,build/%.o,$(SRC))
DEP = $(OBJ:.o=.d)

# Target
TARGET = main

.PHONY: all clean tree

all: $(TARGET)

# Link final binary
$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

# Compile .c to .o in build/
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean everything
clean:
	rm -rf build $(TARGET)

# Print file tree (limit to 3 levels)
tree:
	tree -a -L 3

# Include auto-generated dependency files
-include $(DEP)
