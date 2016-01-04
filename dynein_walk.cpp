#include <stdio.h>
#include <stdlib.h>

#include "dynein_struct.h"
  /*
   * For every timestep, call update_velocities to update internal velocities.
   * Then do Euler's method to update internal coordinates and log output.
   * update_velocities must be called after every set_x command to update
   * internal velocities.
   */

extern const double dt;
double runtime;

void simulateProtein(Dynein* dyn, double tf) {
  double t = 0;

  FILE* data_file = fopen("data.txt", "a+");
  
  while( t < tf ) {
    if (dyn->rand->rand() < dyn->get_unbinding_rate()*dt) {
      dyn->log_run(t);
      dyn->unbind();
      dyn->log(t, data_file);
      exit(EXIT_SUCCESS);
    } else if (dyn->rand->rand() < dyn->get_binding_rate()*dt) {
      printf("Switching states at %g (%.2f%%)\n", t, t/tf*100);
      dyn->switch_to_bothbound();
    }
    
    dyn->update_velocities();

    if (dyn->get_state() != Dynein::BOTHBOUND) {
      double temp_bba,temp_bma, temp_fma, temp_fba;
      temp_bba = dyn->get_bba() + dyn->get_d_bba() * dt;
      temp_bma = dyn->get_bma() + dyn->get_d_bma() * dt;
      temp_fma = dyn->get_fma() + dyn->get_d_fma() * dt;
      temp_fba = dyn->get_fba() + dyn->get_d_fba() * dt;
    
      dyn->set_bba(temp_bba);
      dyn->set_bma(temp_bma);
      dyn->set_fma(temp_fma);
      dyn->set_fba(temp_fba);
    }
    else {
      double temp_nma, temp_fma;
      temp_nma = dyn->get_nma() + dyn->get_d_nma() * dt;
      temp_fma = dyn->get_fma() + dyn->get_d_fma() * dt;
    
      dyn->set_nma(temp_nma);
      dyn->set_fma(temp_fma);
    }
    
    dyn->log(t, data_file);
    
    t += dt;
  }
  dyn->log_run(tf);
  
  fclose(data_file);
}


/* *********************************** MAIN ****************************************** */

int main(int argvc, char **argv) {  
  if (argvc != 6) {
    printf("Error. Usage: ./walk runtime bla_init mla_init mra_init bra_init.\n");
    exit(EXIT_FAILURE);
  }

  runtime  = strtod(argv[1], NULL) * dt;

  equilibrium_angles eq = near_farbound_post_powerstroke_internal_angles;

  double bba_init = strtod(argv[2], NULL) * M_PI + eq.bba;
  double bma_init = strtod(argv[3], NULL) * M_PI + bba_init + eq.ba - M_PI;
  double fma_init = strtod(argv[4], NULL) * M_PI + bma_init + eq.ta;
  double fba_init = strtod(argv[5], NULL) * M_PI + fma_init + M_PI - eq.fa;
  
  Dynein* dyn = new Dynein(bba_init, bma_init, fma_init, fba_init, // Initial angles
			   FARBOUND,                               // Initial state
			   NULL,                // Optional custom internal forces
			   NULL,                // Optional custom brownian forces
			   NULL);               // Optional custom equilibrium angles
  
  dyn->resetLog();
  simulateProtein(dyn, runtime);
  delete dyn;
  dyn = NULL;
  return 0;
}
