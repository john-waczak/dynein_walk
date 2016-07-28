#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../dynein_struct.h"
#include "../default_parameters.h"
#include "simulation_defaults.h"

typedef struct {
  double* bba;
  double* bma;
  double* ta;
  double* uma;
  double* bba_PE;
  double* bma_PE;
  double* ta_PE;
  double* uma_PE;
  double* bbx;     double* bby;
  double* bmx;     double* bmy;
  double* tx;      double* ty;
  double* umx;     double* umy;
  double* ubx;     double* uby;
  double* f_bbx;   double* f_bby;
  double* f_bmx;   double* f_bmy;
  double* f_tx;    double* f_ty;
  double* f_umx;   double* f_umy;
  double* f_ubx;   double* f_uby;
} movie_generate_struct;

void generate_movie(double* time, movie_generate_struct* data, int len, char* fname_base) {
  char fname[200];
  strcpy(fname, fname_base);
  strcat(fname, ".txt");

  FILE* data_file = fopen(fname, "w");
  
  int d_iter = len / movie_num_frames;
  if (d_iter < 1) d_iter = 1;

  for (int iter = 0; iter < len; iter += d_iter) {
    fprintf(data_file,
	    "%d\t"
	    "%.2g\t"
	    "%.2g\t%.2g\t%.2g\t%.2g\t%.2g\t"
	    "%.4f\t%.4f\t%.4f\t%.4f\t"
	    "%.4f\t%.4f\t%.4f\t%.4f\t"
	    "%.4f\t%.4f\t"
	    "%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f"
	    "\n",
	    NEARBOUND,
	    time[iter],
	    data->bba_PE[iter], data->bma_PE[iter], data->ta_PE[iter], data->uma_PE[iter], 0.0,
	    data->bbx[iter], data->bby[iter], data->bmx[iter], data->bmy[iter],
	    data->tx[iter], data->ty[iter], data->umx[iter], data->umy[iter],
	    data->ubx[iter], data->uby[iter],
	    data->f_bbx[iter], data->f_bby[iter], data->f_bmx[iter], data->f_bmy[iter], data->f_tx[iter],
	    data->f_ty[iter], data->f_umx[iter], data->f_umy[iter], data->f_ubx[iter], data->f_uby[iter]);
    printf("Progress on %s: %.1f%%  \r", fname, iter * 100.0 / len);
  }
  printf("Finished %s                \n", fname);
  fclose(data_file);
}

void generate_correlation_fn_data(double* pe, int iters, const char* legend, char* fname_base){
  int max_tau_iter;
  if (iters*tau_runtime_fraction > num_corr_datapoints) {
    max_tau_iter = iters*tau_runtime_fraction;
  }
  else {
    max_tau_iter = num_corr_datapoints;
  }
  int d_tau_iter = max_tau_iter / num_corr_datapoints;

  char fname[200];
  strcpy(fname, fname_base);
  strcat(fname, "_correlation_fn.txt");

  char buf[256];
  sprintf(buf, "--legend='%s'", legend);
  prepare_data_file(buf, fname);

  double* taus =         (double*) malloc(num_corr_datapoints * sizeof(double));
  double* correlations = (double*) malloc(num_corr_datapoints * sizeof(double));

  double pe_ave = get_average(pe, iters);
  double pe_var = get_variance(pe, iters);

  for (int n=0; n<num_corr_datapoints; n++) {
    int tau_iter = n*d_tau_iter;
    double correlation_sum = 0;
    for (int i=0; i<(iters-tau_iter); i++) {
      correlation_sum += (pe[i] - pe_ave) * (pe[i+tau_iter] - pe_ave);
    }
    correlations[n] = correlation_sum / pe_var / (iters-tau_iter);
    taus[n] = tau_iter*dt*pe_calculation_skip_iterations;
    printf("Progress on %s: %.1f%%  \r", fname, n * 100.0 / num_corr_datapoints);
  }
  printf("Finished %s                \n", fname);

  FILE* file = fopen(fname, "a");
  append_data_to_file(taus, correlations, num_corr_datapoints, file);
  fclose(file);

  free (taus); free(correlations);
}

void generate_pe_vs_time_data(double* times, double* pe, int iters, const char* legend, char* fname_base) {
  char fname[200];
  strcpy(fname, fname_base);
  strcat(fname, ".txt");

  char buf[256];
  sprintf(buf, "--legend='%s", legend);
  prepare_data_file(buf, fname);

  double et = 0.5*kb*T;
  double* pe_local_ave = (double*) malloc(iters * sizeof(double));
  for (int i = 0; i < iters; i++) {
    if (i == 0 or i == iters-1) {
      pe_local_ave[i] = pe[i] / et;
    }
    else if (i < pe_averaging_width/2) {
      pe_local_ave[i] = get_average(pe, i*2) / et;
    }
    else if (i == pe_averaging_width/2) {
      pe_local_ave[i] = get_average(&pe[i-pe_averaging_width/2], pe_averaging_width) / et;
    }
    else if ( i > pe_averaging_width/2 and i <= (iters-pe_averaging_width/2-1)){
      pe_local_ave[i] = (pe_local_ave[i-1]*i + pe[i]/et) / (i+1);
    }
    else if (i > (iters-pe_averaging_width/2-1)) {
      pe_local_ave[i] = get_average(&pe[iters-1-(iters-1-i)*2], (iters-1-i)*2) / et;
    }
    else {
      printf("Error in PE local averaging!\n");
      exit(1);
    }
    printf("Progress for %s: %.1f%%  \r", fname, i * 100.0 / iters);
  }
  printf("Finished %s                        \n", fname);

  FILE* file = fopen(fname, "a");
  append_data_to_file(times, pe_local_ave, iters, file);
  fclose(file);

  free(pe_local_ave);
}

void generate_ave_pe_and_log_error_data(double* times, double* pe, int iters, const char* legend, char* fname_base) {
  char fname_ave[200];
  char fname_err[200];

  strcpy(fname_ave, fname_base);
  strcpy(fname_err, fname_base);

  strcat(fname_ave, "_eq_ave.txt");
  strcat(fname_err, "_log_error.txt");

  char buf[256];
  sprintf(buf, "--legend='%s'", legend);
  prepare_data_file(buf, fname_ave);
  sprintf(buf, "--legend='%s'", legend);
  prepare_data_file(buf, fname_err);

  double* pe_aves = (double*) malloc(iters * sizeof(double));
  double* log_err = (double*) malloc(iters * sizeof(double));

  double et = 0.5*kb*T;
  pe_aves[0] = pe[0]/et;
  for (int i=1; i<iters; i++) {
    pe_aves[i] = (pe_aves[i-1]*i + pe[i]/et) / (i+1);
    printf("Progress for %s: %.1f%%  \r", fname_ave, i * 100.0 / iters);
  }
  printf("Finished %s                        \n", fname_ave);

  for (int i=0; i<iters; i++) {
    log_err[i] = std::abs(pe_aves[i] - 1);
    printf("Progress for %s: %.1f%%  \r", fname_err, i * 100.0 / iters);
  }
  printf("Finished %s                       \n", fname_err);

  FILE* file_ave = fopen(fname_ave, "a");
  FILE* file_err = fopen(fname_err, "a");
  append_data_to_file(times, pe_aves, iters, file_ave);
  append_data_to_file(times, log_err, iters, file_err);
  fclose(file_ave);
  fclose(file_err);

  free(pe_aves); free(log_err);
}

void generate_angle_vs_time_data(double* times, double* angle, int iters, const char* legend, char* fname_base, double eq_angle) {
  char fname[200];
  strcpy(fname, fname_base);
  strcat(fname, ".txt");

  char buf[256];
  sprintf(buf, "--legend='%s', --hline='%g'", legend, eq_angle);
  prepare_data_file(buf, fname);

  double* angle_local_ave = (double*) malloc(iters * sizeof(double));
  for (int i = 0; i < iters; i++) {
    if (i == 0 or i == iters-1) {
      angle_local_ave[i] = angle[i];
    }
    else if (i < angle_averaging_width/2) {
      angle_local_ave[i] = get_average(angle, i*2);
    }
    else if (i == angle_averaging_width/2) {
      angle_local_ave[i] = get_average(&angle[i-angle_averaging_width/2], angle_averaging_width);
    }
    else if ( i > angle_averaging_width/2 and i <= (iters-angle_averaging_width/2-1)){
      angle_local_ave[i] = (angle_local_ave[i-1]*i + angle[i]) / (i+1);
    }
    else if (i > (iters-angle_averaging_width/2-1)) {
      angle_local_ave[i] = get_average(&angle[iters-1-(iters-1-i)*2], (iters-1-i)*2);
    }
    else {
      printf("Error in angle local averaging!\n");
      exit(1);
    }
    printf("Progress for %s: %.1f%%  \r", fname, i * 100.0 / iters);
  }
  printf("Finished %s                        \n", fname);

  FILE* file = fopen(fname, "a");
  append_data_to_file(times, angle_local_ave, iters, file);
  fclose(file);

  free(angle_local_ave);
}

int main(int argc, char** argv) {
  if (FP_EXCEPTION_FATAL) {
    feenableexcept(FE_ALL_EXCEPT); // NaN generation kills program
    signal(SIGFPE, FPE_signal_handler);
  }
  if (argc != 2) {
    printf("Error, TITLE variable must have underscores, not spaces.\n");
    exit(1);
  }

  char* f_appended_name = argv[1];
  char data_fname[200];
  char bba_pe_fname_base[200];
  char bma_pe_fname_base[200];
  char  ta_pe_fname_base[200];
  char uma_pe_fname_base[200];
  char bba_fname_base[200];
  char bma_fname_base[200];
  char  ta_fname_base[200];
  char uma_fname_base[200];
  char movie_fname_base[200];

  strcpy(data_fname, "data/onebound_data_");
  strcat(data_fname, f_appended_name);
  strcat(data_fname, ".bin");
  int data_fd = open(data_fname, O_RDONLY);

  if (errno) {
    perror("Failed opening binary data file");
    printf("File name: %s\n", data_fname);
    exit(errno);
  }

  strcpy(bba_pe_fname_base, "data/ob_bba_pe_"); strcat(bba_pe_fname_base, f_appended_name);
  strcpy(bma_pe_fname_base, "data/ob_bma_pe_"); strcat(bma_pe_fname_base, f_appended_name);
  strcpy( ta_pe_fname_base, "data/ob_ta_pe_");  strcat( ta_pe_fname_base, f_appended_name);
  strcpy(uma_pe_fname_base, "data/ob_uma_pe_"); strcat(uma_pe_fname_base, f_appended_name);

  strcpy(bba_fname_base, "data/ob_bba_angle_"); strcat(bba_fname_base, f_appended_name);
  strcpy(bma_fname_base, "data/ob_bma_angle_"); strcat(bma_fname_base, f_appended_name);
  strcpy( ta_fname_base, "data/ob_ta_angle_");  strcat( ta_fname_base, f_appended_name);
  strcpy(uma_fname_base, "data/ob_uma_angle_"); strcat(uma_fname_base, f_appended_name);

  strcpy(movie_fname_base, "data/movie_"); strcat(movie_fname_base, f_appended_name);

  struct stat data_fd_stat;
  fstat(data_fd, &data_fd_stat);
  int len = data_fd_stat.st_size / sizeof(onebound_data_generate_struct);

  onebound_data_generate_struct* data_map;
  data_map = (onebound_data_generate_struct*) mmap(NULL, len*sizeof(onebound_data_generate_struct), PROT_READ, MAP_PRIVATE, data_fd, 0);

  if (data_map == MAP_FAILED) {
    perror("Error using mmap: ");
    exit(EXIT_FAILURE);
  }

  double* time = (double*) malloc(len * sizeof(double));
  for (int j=0; j < len; j++) { time[j] = data_map[j].time; }

  double* bba_pe = (double*) malloc(len * sizeof(double));
  double* bma_pe = (double*) malloc(len * sizeof(double));
  double* ta_pe = (double*) malloc(len * sizeof(double));
  double* uma_pe = (double*) malloc(len * sizeof(double));

  double* bba_angle = (double*) malloc(len * sizeof(double));
  double* bma_angle = (double*) malloc(len * sizeof(double));
  double* ta_angle = (double*) malloc(len * sizeof(double));
  double* uma_angle = (double*) malloc(len * sizeof(double));

  double* bbx = (double*) malloc(len * sizeof(double));
  double* bmx = (double*) malloc(len * sizeof(double));
  double* tx  = (double*) malloc(len * sizeof(double));
  double* umx = (double*) malloc(len * sizeof(double));
  double* ubx = (double*) malloc(len * sizeof(double));

  double* bby = (double*) malloc(len * sizeof(double));
  double* bmy = (double*) malloc(len * sizeof(double));
  double* ty  = (double*) malloc(len * sizeof(double));
  double* umy = (double*) malloc(len * sizeof(double));
  double* uby = (double*) malloc(len * sizeof(double));

  double* f_bbx = (double*) malloc(len * sizeof(double));
  double* f_bmx = (double*) malloc(len * sizeof(double));
  double* f_tx  = (double*) malloc(len * sizeof(double));
  double* f_umx = (double*) malloc(len * sizeof(double));
  double* f_ubx = (double*) malloc(len * sizeof(double));

  double* f_bby = (double*) malloc(len * sizeof(double));
  double* f_bmy = (double*) malloc(len * sizeof(double));
  double* f_ty  = (double*) malloc(len * sizeof(double));
  double* f_umy = (double*) malloc(len * sizeof(double));
  double* f_uby = (double*) malloc(len * sizeof(double));

  for (int j=0; j < len; j++) {
    bba_pe[j] = data_map[j].bba_PE;
    bma_pe[j] = data_map[j].bma_PE;
     ta_pe[j] = data_map[j].ta_PE;
    uma_pe[j] = data_map[j].uma_PE;
    bba_angle[j] = data_map[j].bba;
    bma_angle[j] = data_map[j].bma;
     ta_angle[j] = data_map[j].ta;
    uma_angle[j] = data_map[j].uma;
    bbx[j] = data_map[j].bbx;
    bmx[j] = data_map[j].bmx;
    tx [j] = data_map[j].tx;
    umx[j] = data_map[j].umx;
    ubx[j] = data_map[j].ubx;
    bby[j] = data_map[j].bby;
    bmy[j] = data_map[j].bmy;
    ty [j] = data_map[j].ty;
    umy[j] = data_map[j].umy;
    uby[j] = data_map[j].uby;
    f_bbx[j] = data_map[j].f_bbx;
    f_bmx[j] = data_map[j].f_bmx;
    f_tx[j]  = data_map[j].f_tx;
    f_umx[j] = data_map[j].f_umx;
    f_ubx[j] = data_map[j].f_ubx;
    f_bby[j] = data_map[j].f_bby;
    f_bmy[j] = data_map[j].f_bmy;
    f_ty[j]  = data_map[j].f_ty;
    f_umy[j] = data_map[j].f_umy;
    f_uby[j] = data_map[j].f_uby;  
  }

  generate_correlation_fn_data(bba_pe, len, "Bound binding", bba_pe_fname_base);
  generate_correlation_fn_data(bma_pe, len, "Bound motor", bma_pe_fname_base);
  generate_correlation_fn_data(ta_pe, len, "Tail", ta_pe_fname_base);
  generate_correlation_fn_data(uma_pe, len, "Unbound motor", uma_pe_fname_base);

  generate_pe_vs_time_data(time, bba_pe, len, "Bound binding", bba_pe_fname_base);
  generate_pe_vs_time_data(time, bma_pe, len, "Bound motor", bma_pe_fname_base);
  generate_pe_vs_time_data(time, ta_pe, len, "Tail", ta_pe_fname_base);
  generate_pe_vs_time_data(time, uma_pe, len, "Unbound motor", uma_pe_fname_base);

  generate_ave_pe_and_log_error_data(time, bba_pe, len, "Bound binding", bba_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, bma_pe, len, "Bound motor", bma_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, ta_pe, len, "Tail", ta_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, uma_pe, len, "Unbound motor", uma_pe_fname_base);

  generate_angle_vs_time_data(time, bba_angle, len, "Bound binding", bba_fname_base, onebound_post_powerstroke_internal_angles.bba);
  generate_angle_vs_time_data(time, bma_angle, len, "Bound motor", bma_fname_base, onebound_post_powerstroke_internal_angles.bma);
  generate_angle_vs_time_data(time, ta_angle, len, "Tail", ta_fname_base, onebound_post_powerstroke_internal_angles.ta);
  generate_angle_vs_time_data(time, uma_angle, len, "Unbound motor", uma_fname_base, onebound_post_powerstroke_internal_angles.uma);

  movie_generate_struct movie_data;
  movie_data.bba_PE = bba_pe;
  movie_data.bma_PE = bma_pe;
  movie_data.ta_PE  = ta_pe;
  movie_data.uma_PE = uma_pe;
  movie_data.bba = bba_angle;
  movie_data.bma = bma_angle;
  movie_data.ta  = ta_angle;
  movie_data.uma = uma_angle;
  movie_data.bbx = bbx;      movie_data.bby = bby;
  movie_data.bmx = bmx;      movie_data.bmy = bmy;
  movie_data.tx  = tx;	      movie_data.ty = ty;
  movie_data.umx = umx;      movie_data.umy = umy;
  movie_data.ubx = ubx;      movie_data.uby = uby;
  movie_data.f_bbx = f_bbx;
  movie_data.f_bmx = f_bmx;
  movie_data.f_tx  = f_tx;
  movie_data.f_umx = f_umx;
  movie_data.f_ubx = f_ubx;
  movie_data.f_bby = f_bby;
  movie_data.f_bmy = f_bmy;
  movie_data.f_ty  = f_ty;
  movie_data.f_umy = f_umy;
  movie_data.f_uby = f_uby;
 
  generate_movie(time, &movie_data, len, movie_fname_base);

  free(bba_pe); free(bma_pe); free(ta_pe); free(uma_pe);
  free(bba_angle); free(bma_angle); free(ta_angle); free(uma_angle);
  free(bbx); free(bmx); free(tx); free(umx); free(ubx);
  free(bby); free(bmy); free(ty); free(umy); free(uby);
  free(f_bbx); free(f_bmx); free(f_tx); free(f_umx); free(f_ubx);
  free(f_bby); free(f_bmy); free(f_ty); free(f_umy); free(f_uby);

  free(time);
}