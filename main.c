#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>		// For mkdir

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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

void plot_with_gnuplot(const char *csv_path)
{
	FILE *gnuplot = popen("gnuplot -persistent", "w");
	if (gnuplot == NULL) {
		perror("Failed to open gnuplot");
		return;
	}

	fprintf(gnuplot,
		"set title 'Consensus Time vs Node Count'\n"
		"set xlabel 'Number of Nodes'\n"
		"set ylabel 'Consensus Time'\n"
		"set grid\n"
		"set datafile separator ','\n"
		"set key outside\n"
		"set terminal pngcairo size 1000,600 enhanced font 'Arial,10'\n"
		"set output 'consensus_plot.png'\n"
		"set logscale y\n"
		"set yrange [20:*]\n"
		"set autoscale ymax\n"
		"set xrange [0:100]\n"
		"set xtics 10\n"
		"\n"
		"# Define mellow colors\n"
		"mellow_blue = '#6699CC'\n"
		"mellow_green = '#66CC99'\n"
		"mellow_red = '#CC6666'\n"
		"\n"
		"# Conditional y and error bar functions\n"
		"valid_y(col) = (col > 0 ? col : 1/0.0)  # NaN for invalid\n"
		"valid_err(ycol, ecol) = (ycol > 0 ? ecol : 1/0.0)\n"
		"\n"
		"plot \\\n"
		"    '%s' using 1:(valid_y($2)):(valid_err($2,$3)) with yerrorlines pt 7 ps 1.5 lw 2 lc rgb mellow_blue title 'Erdős-Rényi', \\\n"
		"    '' using 1:(valid_y($4)):(valid_err($4,$5)) with yerrorlines pt 9 ps 1.5 lw 2 lc rgb mellow_green title 'Watts-Strogatz', \\\n"
		"    '' using 1:(valid_y($6)):(valid_err($6,$7)) with yerrorlines pt 5 ps 1.5 lw 2 lc rgb mellow_red title 'Barabási-Albert'\n",
		csv_path);

	pclose(gnuplot);
}

void ensure_dir_exists(const char *path)
{
	struct stat st = { 0 };
	if (stat(path, &st) == -1) {
		if (mkdir(path, 0700) != 0) {
			fprintf(stderr,
				"Error creating directory %s: %s\n", path,
				strerror(errno));
		}
	}
}

void consensus_time_vs_nodes(int runs)
{
	int max_points = 10;

	// Get current timestamp string
	char timestamp[64];
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", t);

	// Compose base directory path with timestamp
	char base_dir[256];
	sprintf(base_dir, "./simls_raw_data/consensus_vs_nodes-%s",
		timestamp);

	ensure_dir_exists("./simls_raw_data");
	ensure_dir_exists(base_dir);

	float **points = malloc(3 * sizeof(float *));
	float **errors = malloc(3 * sizeof(float *));
	for (int i = 0; i < 3; i++) {
		points[i] = malloc(max_points * sizeof(float));
		errors[i] = malloc(max_points * sizeof(float));
	}

	for (int idx = 0; idx < max_points; idx++) {
		int nnodes = (idx + 1) * 10;

		float er_sum = 0.0f, ws_sum = 0.0f, ba_sum = 0.0f;

		float er_max = -1e30f, ws_max = -1e30f, ba_max = -1e30f;
		float er_min = 1e30f, ws_min = 1e30f, ba_min = 1e30f;
		int counted_runs_er = 0;
		int counted_runs_ws = 0;
		int counted_runs_ba = 0;
		for (int run = 0; run < runs; run++) {
			srand(time(NULL) + run + idx * 1000);

			// ER graph
			graph *er = generate_erdos_renyi(nnodes, 0.3f, 0);
			char outdir_er[512];
			sprintf(outdir_er,
				"%s/er=0.3,%d,0_sim=er,2,1_run%d",
				base_dir, nnodes, run);
			ensure_dir_exists(outdir_er);

			opinion_model *sim_er =
			    create_si_async_temporal(er, 2, 1);
			int er_time =
			    run_simulation(sim_er, 2000, 0.001, outdir_er,
					   0);
			if (er_time < 999) {
				er_sum += (float)er_time;
				counted_runs_er++;
				if ((float)er_time > er_max)
					er_max = (float)er_time;
				if ((float)er_time < er_min)
					er_min = (float)er_time;
			} else {

			}
			free_opinion_space(sim_er->opinion_space);
			free_params(sim_er);
			free_graph(sim_er->network);
			free_model(sim_er);

			// WS graph
			graph *ws =
			    generate_watts_strogatz(nnodes, 4, 0.1f, 0);
			char outdir_ws[512];
			sprintf(outdir_ws,
				"%s/ws=0.1,%d,0_sim=ws,2,1_run%d",
				base_dir, nnodes, run);
			ensure_dir_exists(outdir_ws);

			opinion_model *sim_ws =
			    create_si_async_temporal(ws, 2, 1);
			int ws_time =
			    run_simulation(sim_ws, 2000, 0.001, outdir_ws,
					   0);
			if (ws_time < 999) {
				ws_sum += (float)ws_time;
				counted_runs_ws++;
				if ((float)ws_time > ws_max)
					ws_max = (float)ws_time;
				if ((float)ws_time < ws_min)
					ws_min = (float)ws_time;
			}
			free_opinion_space(sim_ws->opinion_space);
			free_params(sim_ws);
			free_graph(sim_ws->network);
			free_model(sim_ws);

			// BA graph
			graph *ba = generate_barabasi_albert(nnodes, 2, 0);
			char outdir_ba[512];
			sprintf(outdir_ba, "%s/ba=2,%d,0_sim=ba,2,1_run%d",
				base_dir, nnodes, run);
			ensure_dir_exists(outdir_ba);

			opinion_model *sim_ba =
			    create_si_async_temporal(ba, 2, 1);
			int ba_time =
			    run_simulation(sim_ba, 2000, 0.001, outdir_ba,
					   0);
			if (ba_time < 999) {
				ba_sum += (float)ba_time;
				counted_runs_ba++;
				if ((float)ba_time > ba_max)
					ba_max = (float)ba_time;
				if ((float)ba_time < ba_min)
					ba_min = (float)ba_time;
			}
			free_opinion_space(sim_ba->opinion_space);
			free_params(sim_ba);
			free_graph(sim_ba->network);
			free_model(sim_ba);
		}
		if (er_sum == 0) {
			printf
			    ("nnodes=%d, er didnt converge for any of the 50 runs\n",
			     nnodes);
		}
		if (ws_sum == 0) {
			printf
			    ("nnodes=%d, ws didnt converge for any of the 50 runs\n",
			     nnodes);
		}
		if (ba_sum == 0) {
			printf
			    ("nnodes=%d, ba didnt converge for any of the 50 runs\n",
			     nnodes);
		}
		points[0][idx] = er_sum / counted_runs_er;
		points[1][idx] = ws_sum / counted_runs_ws;
		points[2][idx] = ba_sum / counted_runs_ba;

		errors[0][idx] = (er_max - er_min) / 2.0f;
		errors[1][idx] = (ws_max - ws_min) / 2.0f;
		errors[2][idx] = (ba_max - ba_min) / 2.0f;
	}

	// Write combined means and errors to one CSV file
	char combined_path[512];
	sprintf(combined_path, "%s/consensus_combined.csv", base_dir);
	FILE *f_combined = fopen(combined_path, "w");
	if (!f_combined) {
		perror("Failed to open consensus_combined.csv");
		goto cleanup;
	}

	fprintf(f_combined,
		"Nodes,ER_mean,ER_error,WS_mean,WS_error,BA_mean,BA_error\n");
	for (int i = 0; i < max_points; i++) {
		fprintf(f_combined, "%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
			(i + 1) * 10,
			points[0][i], errors[0][i],
			points[1][i], errors[1][i],
			points[2][i], errors[2][i]
		    );
	}
	fclose(f_combined);

      cleanup:
	for (int i = 0; i < 3; i++) {
		free(points[i]);
		free(errors[i]);
	}
	free(points);
	free(errors);
	plot_with_gnuplot(combined_path);
}

int main(void)
{
	srand(time(NULL));
	//consensus_time_vs_nodes(10);
	graph *g2 = generate_erdos_renyi(30, 0.3, 0);
	opinion_model *sim = create_si_async_temporal(g2, 2, 1);
	if (sim == NULL) {
		printf("Failed to create model\n");
		return 1;
	}
	char *outdir = create_dir_with_curr_timestamp("simls_raw_data");
	save_graph(g2, "./curr_og.graph");
	int step_count = run_simulation(sim, 10000, 0.001, outdir, 1);
	size_t len = strlen(outdir) + strlen("/graph.layout") + 1;
	char *layout_path = malloc(len);

	snprintf(layout_path, len, "%s/graph.layout", outdir);
	if (step_count < 999) {
		char images_dir[512];
		snprintf(images_dir, sizeof(images_dir), "%s/images",
			 outdir);
		mkdir(images_dir, 0755);
		for (int i = 0; i <= step_count; i++) {
			printf("\rGenerating picture %d/%d", i,
			       step_count);
			fflush(stdout);
			char graph_path[512];
			char opinion_path[512];
			char img_path[512];
			snprintf(graph_path, sizeof(graph_path),
				 "%s/%d.graph", outdir, i);
			snprintf(opinion_path, sizeof(opinion_path),
				 "%s/%d.opinions", outdir, i);
			snprintf(img_path, sizeof(img_path), "%s/%04d.png",
				 images_dir, i);
			save_image_as_graph_wopinion_labels_and_colors
			    (graph_path, opinion_path, img_path,
			     layout_path, g2->num_nodes);
		}
	}
	generate_video_from_images(outdir, NULL, 20);
	free_opinion_space(sim->opinion_space);
	free_params(sim);
	free_model(sim);
	free_graph(g2);
	free(outdir);
	close_urandom();
	return 0;
}
