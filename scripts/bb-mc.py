import numpy as np
import matplotlib.pyplot as plt
import scipy.constants
import sys
sys.path.append("../data")
import importlib
import argparse
import subprocess
import bb_energy_distribution

params = importlib.import_module("params")

parser = argparse.ArgumentParser()
parser.add_argument("-L", type=float, help="displacement in nm", required=True)
args = parser.parse_args()

angles = [[] for i in range(2)]                # Pair of angles

rate_unbinding_leading = []                        # Leading (Far) Unbinding Rates
rate_unbinding_trailing = []                # Trailing (Near) Unbinding Rates


C =  params.for_simulation['exp-unbinding-constant']         # exponential binding constant from paper_params.py April 12

Z = 0                # Partition Function
N = 100                 # Count
L = args.L         # Length

# These are sums of partition function
r_tx = 0                # Tail x position
r_ty = 0                # Tail y position
r_nmx = 0                # Near motor x position
r_fmx = 0                # Far motor x position
r_fbx = 0                # Far bound x position
E_avg = 0                # Energy Average

b = 1                        # thermodynamic beta

max_rate_trailing = 0
max_rate_leading = 0

trailing_data = [[] for i in range(2)]
leading_data = [[] for i in range(2)]

eqb_angle = params.for_simulation['eqb']
if bb_energy_distribution.eq_in_degrees:
        eqb_angle = eqb_angle*np.pi/180

seed = 0 # The initial seed for the C++ onebound code.
np.random.seed(0)

def run_onebound(bba, bma, uma, uba):
        global seed
        seed += 1 # use a different seed every time.  ugh, global variables!
        print('running with inputs', bba, bma, uma, uba)
        process = subprocess.Popen(['../onebound',
                                    str(params.for_simulation['k_b']),
                                    str(params.for_simulation['cb']),
                                    str(params.for_simulation['cm']),
                                    str(params.for_simulation['ct']),
                                    str(params.for_simulation['ls']),
                                    str(params.for_simulation['lt']),
                                    str(params.for_simulation['rt']),
                                    str(params.for_simulation['rm']),
                                    str(params.for_simulation['rb']),
                                    str(seed), # FIXME
                                    str(params.for_simulation['dt']),
                                    str(params.for_simulation['eqb']),
                                    str(params.for_simulation['eqmpre']),
                                    str(params.for_simulation['eqmpost']),
                                    str(params.for_simulation['eqt']),
                                    str(params.for_simulation['force']),
                                    str(params.for_simulation['exp-unbinding-constant']),
                                    str(bba), str(bma), str(uma), str(uba),
        ], stdout=subprocess.PIPE)
        (output, err) = process.communicate()
        exit_code = process.wait()
        output_data = eval(output.decode('utf-8'))
        # print('onebound for a step (which kind?) gives:\n%s\nEND OUTPUT' % output.decode('utf-8'))
        # print('data', output_data)
        assert(exit_code == 0);
        return output_data

while Z < N:
        # Making random motor angles
        nma = np.random.uniform(0, 2*np.pi)
        fma = np.random.uniform(0, 2*np.pi)
        angles[0].append(nma)
        angles[1].append(fma)

        dynein = bb_energy_distribution.DyneinBothBound(nma, fma, params, L)

        # Checking if energy is nan
        if np.isnan(dynein.E_total) == True:
                continue
        else:
                # Calculating partition function
                P = np.exp(-b*dynein.E_total)
                Z += P

                # Calculation for averages
                r_tx += dynein.r_t[0]*P
                r_ty += dynein.r_t[1]*P
                r_nmx += dynein.r_nm[0]*P
                r_fmx += dynein.r_fm[0]*P
                r_fbx += dynein.r_fb[0]*P
                E_avg += dynein.E_total*P

                rate_trailing = np.exp(C*(dynein.nba - eqb_angle))
                rate_leading = np.exp(C*(dynein.fba - eqb_angle))
                rate_unbinding_trailing.append(rate_trailing)
                rate_unbinding_leading.append(rate_leading)
                max_rate_leading = max(rate_leading, max_rate_leading)
                max_rate_trailing = max(rate_trailing, max_rate_trailing)

                prob_trailing = P*rate_trailing
                prob_leading = P*rate_leading

                # print("prob_leading: ", prob_leading)
                # print("prob_trailing: ", prob_trailing)

                new_nma = nma-(np.pi-dynein.nba)
                new_fma = fma-(np.pi-dynein.fba)

                # bba_old = np.pi - dynein.nba - nma
                # uba_old = np.pi - dynein.fba - fma
                # print("bba: {0}  bba_old: {1}".format( bba, bba_old))
                # print("uba: {0}  uba_old: {1}".format( uba, uba_old))

                if np.random.random() < prob_trailing: # FIXME need to normalize this a tad so it is never > 1.
                        print('\n\ntrailing',dynein.nba,
                                            new_nma,
                                            new_fma,
                                            dynein.fba)
                        step = run_onebound(dynein.nba,
                                            new_nma,
                                            new_fma,
                                            dynein.fba)
                        trailing_data[0].append(step['L'])
                        trailing_data[1].append(step['t'])
                        print('trailing stepped with final displacement %g after time %g \n' % (step['L'], step['t']))
                if np.random.random() < prob_leading:
                        print('\n\nleading', dynein.fba, new_fma,
                                            new_nma,
                                            dynein.nba)
                        step = run_onebound(dynein.fba,
                                            new_fma,
                                            new_nma,
                                            dynein.nba)
                        leading_data[0].append(step['L'])
                        leading_data[1].append(step['t'])
                        print('leading stepped with final displacement %g after time %g \n' % (step['L'], step['t']))


# What to collect and output or visualize?

### Bothbound data
# Mean angles while bothbound? (no stepping required)
# Mean motor/tail domain locations


### Stepping data (separately for leading/trailing)
# Final displacement (mean/histogram/list)
# Onebound time (mean/histogram/list)
# Rate of stepping

# print("rate_unbinding_leading: ", rate_unbinding_leading)
# print("rate_unbinding_trailing: ", rate_unbinding_trailing)
# print('max_rate_trailing', max_rate_trailing)
# print('max_rate_leading', max_rate_leading)

### What to export, and in what format?
# Histograms of final displacements?

tx = r_tx/Z          # Tail x array
ty = r_ty/Z          # Tail y array
nmx = r_nmx/Z        # Near motor array
fmx = r_fmx/Z        # Far motor array
fbx = r_fbx/Z        # Far bound array
E_avg_arr = E_avg/Z  # Average energy array

print("Avg Tail x:", tx)
print("Avg Tail y:", ty)
print("Avg nmx:", nmx)
print("Avg fmx:", fmx)
print("Avg fbx:", fbx)
print("Avg E:", E_avg_arr)
