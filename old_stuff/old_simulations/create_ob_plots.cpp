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
#include "plotting_defaults.h"

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
  char* data_fname = new char[200];
  char* bba_pe_fname_base = new char[200];
  char* bma_pe_fname_base = new char[200];
  char*  ta_pe_fname_base = new char[200];
  char* uma_pe_fname_base = new char[200];
  char* total_pe_fname_base = new char[200];

  char* bba_force_fname_base = new char[200];
  char* bma_force_fname_base = new char[200];
  char*  ta_force_fname_base = new char[200];
  char* uma_force_fname_base = new char[200];
  char* uba_force_fname_base = new char[200];
  char* total_force_fname_base = new char[200];

  char* bba_fname_base = new char[200];
  char* bma_fname_base = new char[200];
  char*  ta_fname_base = new char[200];
  char* uma_fname_base = new char[200];
  char* everything_name = new char[200];

  strcpy(data_fname, "data/onebound_data_");
  strcat(data_fname, f_appended_name);
  strcat(data_fname, ".bin");
  int data_fd = open(data_fname, O_RDONLY);

  if (errno) {
    perror("Failed opening binary data file");
    printf("File name: %s\n", data_fname);
    exit(errno);
  }

  sprintf(everything_name, "data/everything_%s.txt", f_appended_name);

  sprintf(bba_pe_fname_base, "data/ob_bba_pe_%s", f_appended_name);
  sprintf(bma_pe_fname_base, "data/ob_bma_pe_%s", f_appended_name);
  sprintf( ta_pe_fname_base, "data/ob_ta_pe_%s",  f_appended_name);
  sprintf(uma_pe_fname_base, "data/ob_uma_pe_%s", f_appended_name);
  sprintf(total_pe_fname_base, "data/ob_total_pe_%s", f_appended_name);

  sprintf(bba_force_fname_base, "data/ob_bba_force_%s", f_appended_name);
  sprintf(bma_force_fname_base, "data/ob_bma_force_%s", f_appended_name);
  sprintf( ta_force_fname_base, "data/ob_ta_force_%s",  f_appended_name);
  sprintf(uma_force_fname_base, "data/ob_uma_force_%s", f_appended_name);
  sprintf(uba_force_fname_base, "data/ob_uba_force_%s", f_appended_name);
  sprintf(total_force_fname_base, "data/ob_total_force_%s", f_appended_name);

  sprintf(bba_fname_base, "data/ob_bba_angle_%s", f_appended_name);
  sprintf(bma_fname_base, "data/ob_bma_angle_%s", f_appended_name);
  sprintf( ta_fname_base, "data/ob_ta_angle_%s",  f_appended_name);
  sprintf(uma_fname_base, "data/ob_uma_angle_%s", f_appended_name);

  struct stat data_fd_stat;
  fstat(data_fd, &data_fd_stat);
  int len = data_fd_stat.st_size / sizeof(onebound_data_generate_struct);

  onebound_data_generate_struct* data_map;
  data_map = (onebound_data_generate_struct*) mmap(NULL, len*sizeof(onebound_data_generate_struct), PROT_READ, MAP_SHARED, data_fd, 0);
  if (data_map == MAP_FAILED) {
    perror("Error using mmap: ");
    exit(EXIT_FAILURE);
  }

  for (int i=0; i<len; i++) { // set the length to data size, in case simulation not complete
    if (data_map[i].bbx == 0 && data_map[i].bmx == 0 && data_map[i].tx == 0 && data_map[i].umx == 0 && data_map[i].ubx == 0) {
      len = i;
      break;
    }
  }

  {
    FILE *f = fopen(everything_name, "w");
    fprintf(f, "# T: %g\n", 50.0); // UGH FIXME!!!
    fprintf(f, "# kT: %g\n", kb*50.0); // UGH FIXME!!!
    fprintf(f, "# onebound.bba: %g\n", onebound_post_powerstroke_internal_angles.bba);
    fprintf(f, "# onebound.bma: %g\n", onebound_post_powerstroke_internal_angles.bma);
    fprintf(f, "# onebound.ta: %g\n", onebound_post_powerstroke_internal_angles.ta);
    fprintf(f, "# onebound.uma: %g\n", onebound_post_powerstroke_internal_angles.uma);
    fprintf(f, "# %s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
            "time",
            "bba_PE",
            "bma_PE",
            "ta_PE",
            "uma_PE",
            "bba",
            "bma",
            "ta",
            "uma",
            "bbx",
            "bmx",
            "tx",
            "umx",
            "ubx",
            "bby",
            "bmy",
            "ty",
            "umy",
            "uby",
            "f_bbx",
            "f_bmx",
            "f_tx",
            "f_umx",
            "f_ubx",
            "f_bby",
            "f_bmy",
            "f_ty",
            "f_umy",
            "f_uby");
    for (int j=0;j<len;j++) {
      fprintf(f, "%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\n",
              data_map[j].time,
              data_map[j].bba_PE,
              data_map[j].bma_PE,
              data_map[j].ta_PE,
              data_map[j].uma_PE,
              data_map[j].bba,
              data_map[j].bma,
              data_map[j].ta,
              data_map[j].uma,
              data_map[j].bbx,
              data_map[j].bmx,
              data_map[j].tx,
              data_map[j].umx,
              data_map[j].ubx,
              data_map[j].bby,
              data_map[j].bmy,
              data_map[j].ty,
              data_map[j].umy,
              data_map[j].uby,
              data_map[j].f_bbx,
              data_map[j].f_bmx,
              data_map[j].f_tx,
              data_map[j].f_umx,
              data_map[j].f_ubx,
              data_map[j].f_bby,
              data_map[j].f_bmy,
              data_map[j].f_ty,
              data_map[j].f_umy,
              data_map[j].f_uby);
    }
    fclose(f);
  }

  double* time = (double*) malloc(len * sizeof(double));
  for (int j=0; j < len; j++) { time[j] = data_map[j].time; }

  double* bba_pe = (double*) malloc(len * sizeof(double));
  double* bma_pe = (double*) malloc(len * sizeof(double));
  double* ta_pe = (double*) malloc(len * sizeof(double));
  double* uma_pe = (double*) malloc(len * sizeof(double));
  double* total_pe = (double*) malloc(len * sizeof(double));

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

  double* f_total_x = (double*) malloc(len * sizeof(double));
  double* f_total_y = (double*) malloc(len * sizeof(double));

  for (int j=0; j < len; j++) {
    bba_pe[j] = data_map[j].bba_PE;
    bma_pe[j] = data_map[j].bma_PE;
     ta_pe[j] = data_map[j].ta_PE;
    uma_pe[j] = data_map[j].uma_PE;
    total_pe[j] = (bba_pe[j] + bma_pe[j] + ta_pe[j] + uma_pe[j]) / 4;
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
    f_total_x[j] = f_bbx[j] + f_bmx[j] + f_tx[j] + f_umx[j] + f_ubx[j];
    f_total_y[j] = f_bby[j] + f_bmy[j] + f_ty[j] + f_umy[j] + f_uby[j];
  }

  generate_correlation_fn_data(bba_pe, len, "Bound binding", bba_pe_fname_base);
  generate_correlation_fn_data(bma_pe, len, "Bound motor", bma_pe_fname_base);
  generate_correlation_fn_data(ta_pe, len, "Tail", ta_pe_fname_base);
  generate_correlation_fn_data(uma_pe, len, "Unbound motor", uma_pe_fname_base);
  generate_correlation_fn_data(total_pe, len, "Total", total_pe_fname_base);

  generate_pe_vs_time_data(time, bba_pe, len, "Bound binding PE", bba_pe_fname_base);
  generate_pe_vs_time_data(time, bma_pe, len, "Bound motor PE", bma_pe_fname_base);
  generate_pe_vs_time_data(time, ta_pe, len, "Tail PE", ta_pe_fname_base);
  generate_pe_vs_time_data(time, uma_pe, len, "Unbound motor PE", uma_pe_fname_base);
  generate_pe_vs_time_data(time, total_pe, len, "Total PE", total_pe_fname_base);

  generate_ave_pe_and_log_error_data(time, bba_pe, len, "Bound binding", bba_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, bma_pe, len, "Bound motor", bma_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, ta_pe, len, "Tail", ta_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, uma_pe, len, "Unbound motor", uma_pe_fname_base);
  generate_ave_pe_and_log_error_data(time, total_pe, len, "Total", total_pe_fname_base);

  generate_angle_vs_time_data(time, bba_angle, len, "Bound binding angle", bba_fname_base, onebound_post_powerstroke_internal_angles.bba);
  generate_angle_vs_time_data(time, bma_angle, len, "Bound motor angle", bma_fname_base, onebound_post_powerstroke_internal_angles.bma);
  generate_angle_vs_time_data(time, ta_angle, len, "Tail angle", ta_fname_base, onebound_post_powerstroke_internal_angles.ta);
  generate_angle_vs_time_data(time, uma_angle, len, "Unbound motor angle", uma_fname_base, onebound_post_powerstroke_internal_angles.uma);

  generate_force_data(time, f_bbx, len, "Bound binding f_x", bba_force_fname_base, "x");
  generate_force_data(time, f_bmx, len, "Bound motor f_x", bma_force_fname_base, "x");
  generate_force_data(time, f_tx,  len, "Tail f_x", ta_force_fname_base, "x");
  generate_force_data(time, f_umx, len, "Unbound motor f_x", uma_force_fname_base, "x");
  generate_force_data(time, f_ubx, len, "Unbound binding f_x", uba_force_fname_base, "x");
  generate_force_data(time, f_total_x, len, "Total f_x", total_force_fname_base, "x");

  generate_force_data(time, f_bby, len, "Bound binding f_y", bba_force_fname_base, "y");
  generate_force_data(time, f_bmy, len, "Bound motor f_y", bma_force_fname_base, "y");
  generate_force_data(time, f_ty,  len, "Tail f_y", ta_force_fname_base, "y");
  generate_force_data(time, f_umy, len, "Unbound motor f_y", uma_force_fname_base, "y");
  generate_force_data(time, f_uby, len, "Unbound binding f_y", uba_force_fname_base, "y");
  generate_force_data(time, f_total_y, len, "Total f_y", total_force_fname_base, "y");

  free(bba_pe); free(bma_pe); free(ta_pe); free(uma_pe);
  free(bba_angle); free(bma_angle); free(ta_angle); free(uma_angle);
  free(bbx); free(bmx); free(tx); free(umx); free(ubx);
  free(bby); free(bmy); free(ty); free(umy); free(uby);
  free(f_bbx); free(f_bmx); free(f_tx); free(f_umx); free(f_ubx);
  free(f_bby); free(f_bmy); free(f_ty); free(f_umy); free(f_uby);

  free(time);
}
