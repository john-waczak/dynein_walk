#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dynein_struct.h"

/*
 * For every timestep, call update_velocities to update internal velocities.
 * Then do Euler's method to update internal coordinates and log output.
 * update_velocities must be called after every set_a command to update
 * internal velocities.
 */

extern const double dt;
double runtime;

void simulateProtein(Dynein_onebound* dyn1, double tf) {

}


/* *********************************** MAIN ****************************************** */

int main(int argvc, char **argv) {
  if (argvc != 6) {
    printf("Error. Usage: ./walk runtime bla_init mla_init mra_init bra_init.\n");
    exit(EXIT_FAILURE);
  }

  runtime  = strtod(argv[1], NULL) * dt;

  onebound_equilibrium_angles eq = onebound_post_powerstroke_internal_angles;

  double bba_init = strtod(argv[2], NULL) * M_PI + eq.bba;
  double bma_init = strtod(argv[3], NULL) * M_PI + bba_init + eq.bma - M_PI;
  double uma_init = strtod(argv[4], NULL) * M_PI + bma_init + eq.ta;
  double uba_init = strtod(argv[5], NULL) * M_PI + uma_init + M_PI - eq.uma;

  MTRand* rand = new MTRand(RAND_INIT_SEED);
  Dynein_onebound* dyn_ob = new Dynein_onebound(
				    bba_init, bma_init, uma_init, uba_init, // Initial angles
				    0, 0,                // Starting coordinates
				    FARBOUND,            // Initial state
				    NULL,                // Optional custom internal forces
				    NULL,                // Optional custom brownian forces
				    NULL,                // Optional custom equilibrium angles
                                    rand);
  Dynein_bothbound *dyn_bb = 0;

  double t = 0;

  double distance_traveled = 0;
  double run_length = 0;
  double steps = 0;

  FILE* data_file = fopen("data.txt", "w");
  FILE* run_file = fopen("run.txt", "w");
  FILE* config_file = fopen("config.txt", "w");

  resetLogs(data_file, config_file);

  while( t < runtime ) {
    while (t < runtime) { // loop as long as it is onebound
      if (rand->rand() < dyn_ob->get_unbinding_rate()*dt) {
        // this is the case where we fall off and become zerobound!
        printf("unbinding.");
        dyn_ob->log(t, data_file);
	goto end_simulation;
        return EXIT_SUCCESS;
      } else if (rand->rand() < dyn_ob->get_binding_rate()*dt) {
        // switch to bothbound
	steps++;
	distance_traveled += fabs(dyn_ob->get_ubx() - dyn_ob->get_bbx());
	run_length = (dyn_ob->get_bbx() + dyn_ob->get_ubx()) / 2.0;

        dyn_bb = new Dynein_bothbound(dyn_ob, rand);
        delete dyn_ob;
        break;
      } else { // move like normal
        double temp_bba = dyn_ob->get_bba() + dyn_ob->get_d_bba() * dt;
        double temp_bma = dyn_ob->get_bma() + dyn_ob->get_d_bma() * dt;
        double temp_uma = dyn_ob->get_uma() + dyn_ob->get_d_uma() * dt;
        double temp_uba = dyn_ob->get_uba() + dyn_ob->get_d_uba() * dt;

        dyn_ob->set_bba(temp_bba);
        dyn_ob->set_bma(temp_bma);
        dyn_ob->set_uma(temp_uma);
        dyn_ob->set_uba(temp_uba);
      }
      dyn_ob->update_velocities();
      dyn_ob->log(t, data_file);
      t += dt;
    }

    printf("switch to bothbound!\n");
    while (t < runtime) { // loop as long as it is bothbound
      bool unbind_near = rand->rand() < dyn_bb->get_near_unbinding_rate()*dt;
      bool unbind_far = rand->rand() < dyn_bb->get_far_unbinding_rate()*dt;
      if (unbind_near && unbind_far) {
        printf("THEY BOTH WANT TO FALL OFF TOGETHER!!!\n");
        printf("WHAT SHOULD THEY DO????\n");
      }
      if (unbind_near) {
        // switch to farbound
	dyn_ob = new Dynein_onebound(dyn_bb, rand, FARBOUND);
	delete dyn_bb;
        break;
      } else if (unbind_far) {
        // switch to nearbound
	dyn_ob = new Dynein_onebound(dyn_bb, rand, NEARBOUND);
	delete dyn_bb;
        break;
      } else {
	double temp_nma = dyn_bb->get_nma() + dyn_bb->get_d_nma()*dt;
	double temp_fma = dyn_bb->get_fma() + dyn_bb->get_d_fma()*dt;

	dyn_bb->set_nma(temp_nma);
	dyn_bb->set_fma(temp_fma);
      }

      dyn_bb->update_velocities();
      dyn_bb->log(t, data_file);
      t += dt;
    }
    printf("switch to onebound!\n");
  }

 end_simulation: log_run(run_file, t, run_length, distance_traveled, steps); // dontkillme
  fclose(data_file);
  fclose(run_file);
  fclose(config_file);

  return 0;
}
