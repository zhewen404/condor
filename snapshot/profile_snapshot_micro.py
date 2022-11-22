import sys
sys.path.append('..')
from util.spec2017_util import (
    bcolors,
    create_folder,
    yaml_load,
    yaml_overwrite,
)
import argparse, subprocess, os, re
from tqdm import tqdm
from scipy.stats import gmean

parser = argparse.ArgumentParser(description="config.")
parser.add_argument(
    "--dir",
    type=str,
    required=True,
    help="dir that contains cache dump",
)
args = parser.parse_args()

result_dir = args.dir
data_file = args.dir

num_sample = 1
out_dir = os.getcwd() + '/out/'
log_dir = os.getcwd() + '/log/'
err_dir = os.getcwd() + '/err/'
create_folder(out_dir)
create_folder(err_dir)
create_folder(log_dir)

f_time_pf = open(log_dir+f"/micro.time", "w+")
f_time_pf_both = open(log_dir+f"/micro.time_both", "w+")

for scheme in tqdm(["bdi", "dedup", "bdiuc", "dedupuc"], desc="compression scheme", leave=True):
    # check binary exists
    if not os.path.isfile(f"{scheme}"): 
        print(bcolors.RED + f"{scheme} binary file not exist!" + bcolors.ENDC)
        exit(1)
    num_snapshots=[]
    for xor_scheme in tqdm(["none", "rand", "ideal"], desc="xor scheme", leave=False):
        cr = []
        cr_both =[]
        if not os.path.isdir(data_file): 
            print(bcolors.RED + f"data dir not exist! {data_file}" + bcolors.ENDC)
            continue
        cmd_list = [f"./{scheme}"] + [data_file] + [xor_scheme]
        p = subprocess.Popen(cmd_list, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(bcolors.OKGREEN + f'micro Launched:  ' + bcolors.ENDC + f"{scheme}+{xor_scheme}")
        print(f'{p.args}')
        out,err = p.communicate()
        out = out.decode('utf-8')
        err = err.decode('utf-8')
        print(out)
        if err: 
            print(bcolors.RED + err + bcolors.ENDC)
            ferr = open(f"err/micro.{scheme}_{xor_scheme}","w")
            ferr.write(err)
            ferr.write(f'{p.args}')
            ferr.close()
        else:
            cr_ = float(out.split('(')[-1].split(')')[0])
            cr_both_ = float(out.split('{')[-1].split('}')[0])
            print(cr_)
            cr.append(cr_)
            cr_both.append(cr_both_)
        cr_arith = sum(cr)/len(cr)
        cr_geo = gmean(cr)
        num_snapshots.append(len(cr))
        print(f"total {len(cr)} snapshots.")

        # write means and #snapshots to output
        f = open(out_dir+f"/micro.{scheme}_{xor_scheme}", "w")
        f.write(f"{cr_arith}, {cr_geo}, {len(cr)}")
        f.close()

        cr_arith_both = sum(cr_both)/len(cr_both)
        cr_geo_both = gmean(cr_both)
        f_both = open(out_dir+f"/micro.{scheme}_{xor_scheme}_both", "w")
        f_both.write(f"{cr_arith_both}, {cr_geo_both}, {len(cr_both)}")
        f_both.close()

        # write time elaps profile to log
        for i in range(len(cr)): f_time_pf.write(f"{cr[i]} ")
        f_time_pf.write("\n")
        for i in range(len(cr_both)): f_time_pf_both.write(f"{cr_both[i]} ")
        f_time_pf_both.write("\n")

    print(num_snapshots)

f_time_pf.close()
f_time_pf_both.close()
