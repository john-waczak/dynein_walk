#/usr/bin/python3
import subprocess, os
import numpy as np

def have_slurm():
    if avoid_slurm:
        return False
    try:
        subprocess.check_call("squeue > /dev/null", shell=True)
    except (OSError, subprocess.CalledProcessError):
        print("Not using slurm...")
        return False
    return True

def read_csv(fname):
    values = np.loadtxt(fname, delimiter=',', comments='#', dtype='string')
    custom_runs = []
    if values.ndim != 1:
        for i in range(0, values.shape[0]):
            custom_runs.append({"ls":float(values[i,0]), "lt":float(values[i,1]),
                                "k_b":float(values[i,2]), "k_ub":float(values[i,3]),
                                "T":float(values[i,4]), "cb":float(values[i,5]),
                                "cm":float(values[i,6]), "ct":float(values[i,7])})
    else:
        custom_runs.append({"ls":float(values[0]), "lt":float(values[1]),
                            "k_b":float(values[2]), "k_ub":float(values[3]),
                            "T":float(values[4]), "cb":float(values[5]),
                            "cm":float(values[6]), "ct":float(values[7])})
    return custom_runs

def run_sim(**run):
    if os.path.isdir('run_scripts'):
        os.chdir('run_scripts')
    os.system('mkdir -p ../runlogs ../data')
    assert(subprocess.call("cd .. && make histogram-stuff", shell=True) == 0)

    cmd = ["srun"] if have_slurm() else []
    cmd.extend(["nice", "-19"])
    cmd.extend(["./generate_stepping_data"])

    for key in ["ls", "lt", "k_b", "k_ub", "cb", "cm", "ct", "T", "dt", "label", "seed", "runtime", "movie", "onebound-debugging", "constant-write"]:
        if key in run:
            cmd.extend(["--"+key, str(run[key])])
    for key in ["nomovie", "onebound-debugging", "constant-write"]:
        if key in run:
            cmd.extend(["--"+key])

    if 'label' in run:
      basename = "%s__k_b-%g,k_ub-%g,cb-%g,cm-%g,ct-%g,dt-%g" % (str(run["label"]), run["k_b"], run["k_ub"],
                                                                 run["cb"], run["cm"], run["ct"], run["dt"])
    else:
      basename = "k_b-%g,k_ub-%g,cb-%g,cm-%g,ct-%g,dt-%g" % (str(run["label"]), run["k_b"], run["k_ub"],
                                                             run["cb"], run["cm"], run["ct"], run["dt"])

    out = open('../runlogs/' + basename + '.out', 'w')
    process_object = subprocess.Popen(cmd, stdout=out, stderr=subprocess.STDOUT, cwd="../")
    print("Running: ", " ".join(cmd))
    if "no-slurm" in run:
        process_object.wait()
    return basename
