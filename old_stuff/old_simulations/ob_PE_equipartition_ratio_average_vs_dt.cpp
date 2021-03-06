#include <cassert>
#include <stdio.h>
#include <string.h>

#include "../dynein_struct.h"
#include "../default_parameters.h"

int main(int argc, char** argv) {
  if (FP_EXCEPTION_FATAL) {
    feenableexcept(FE_ALL_EXCEPT); // NaN generation kills program
    signal(SIGFPE, FPE_signal_handler);
  }

  double time = 1e-3;
  Lt = 15;
  Ls = 15;
  fake_radius_t = 1.5;
  fake_radius_m = 1.5;
  fake_radius_b = 1.5;
  gt = fake_radius_t*6*M_PI*water_viscosity_mu; // kg / s
  gm = fake_radius_m*6*M_PI*water_viscosity_mu; // kg / s
  gb = fake_radius_b*6*M_PI*water_viscosity_mu; // kg / s

  cb = 1.5;
  cm = 1.5;
  ct = 1.5;

  const char* bba_eq_title = "Bound binding";
  const char* bma_eq_title = "Bound motor";
  const char* ta_eq_title =  "Tail domain";
  const char* uma_eq_title = "Unbound motor";

  if (argc != 2) {
    printf("Error, TITLE variable must have underscores, not spaces.\n");
    exit(1);
  }
  char* f_appended_name = argv[1];
  char bba_eq_fname[200];
  char bma_eq_fname[200];
  char ta_eq_fname[200];
  char uma_eq_fname[200];

  strcpy(bba_eq_fname, "data/bba_pe_equipartition_ratio_vs_dt_");
  strcpy(bma_eq_fname, "data/bma_pe_equipartition_ratio_vs_dt_");
  strcpy(ta_eq_fname, "data/ta_pe_equipartition_ratio_vs_dt_");
  strcpy(uma_eq_fname, "data/uma_pe_equipartition_ratio_vs_dt_");
  strcpy(config_eq_fname, "data/config_pe_equipartition_ratio_vs_dt_");

  strcat(bba_eq_fname, f_appended_name);
  strcat(bma_eq_fname, f_appended_name);
  strcat(ta_eq_fname, f_appended_name);
  strcat(config_eq_fname, f_appended_name);

  strcat(bba_eq_fname, ".txt");
  strcat(bma_eq_fname, ".txt");
  strcat(ta_eq_fname, ".txt");
  strcat(uma_eq_fname, ".txt");
  strcat(config_eq_fname, ".txt");

  const int seeds[] = {0, 1};
  int seed_len = sizeof(seeds) / sizeof(int);

  const double dts[] = {1e-12, 5e-12, 1e-11,
			1.5e-11, 2.5e-11, 5e-11, 6e-11, 7.5e-11, 9e-11, 1e-10, 2.5e-10,
			3.5e-10, 5e-10, 5e-10, 7.5e-10, 9e-10, 1e-9, 2.5e-9};
  int num_dts = sizeof(dts) / sizeof(double);
  
  char run_msg[512];
  const char* run_msg_base = "dt calc (";

  prepare_data_file(bba_eq_title, bba_eq_fname);
  prepare_data_file(bma_eq_title, bma_eq_fname);
  prepare_data_file(ta_eq_title,  ta_eq_fname);
  prepare_data_file(uma_eq_title, uma_eq_fname);

  for (int i = 0; i < num_dts; i++) {
    dt = dts[i];

    onebound_data eq_data;
    double bb_double, bm_double, t_double, um_double;
    eq_data.len = 1;
    eq_data.bb = &bb_double;
    eq_data.bm = &bm_double;
    eq_data.t =  &t_double;
    eq_data.um = &um_double;

    onebound_forces force_var_data_struct;
    generic_data unused_force_var_data;
    unused_force_var_data.len = 1;
    unused_force_var_data.data = &force_var_data_struct;

    strcpy(run_msg, run_msg_base);
    char buf[50];
    sprintf(buf, "dt = %g, ", dt);
    strcat(run_msg, buf);

    int iterations = time / dt;
    printf("time: %g, dt: %g, iterations: %d\n", time, dt, iterations);
    
    get_onebound_equipartition_ratio(
	  &eq_data, &unused_force_var_data, iterations, seeds, seed_len, run_msg);

    append_data_to_file(&dt, eq_data.bb, 1, bba_eq_fname);
    append_data_to_file(&dt, eq_data.bm, 1, bma_eq_fname);
    append_data_to_file(&dt, eq_data.t , 1, ta_eq_fname);
    append_data_to_file(&dt, eq_data.um, 1, uma_eq_fname);
  }

  write_config_file(config_eq_fname, NULL, NULL);
  return 0;
}
