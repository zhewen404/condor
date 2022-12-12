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
    "--bench_num",
    type=int,
    required=True,
    help="spec benchmark num",
)
args = parser.parse_args()

gem5_dir = "/home/zhewen/repo/gem5-dev/gem5"

yml_file = "../util/spec2017_default.yml"
spec2017 = yaml_load(yml_file)
bench_name = spec2017[args.bench_num]["name"]
ckpt = gem5_dir + f"/ckpt/spec2017/1c-2GB-valgrind/{args.bench_num}.{bench_name}"

from glob import glob
num_ckpt = len(glob(os.path.join(ckpt, "cpt.*", "")))
print(f"spec {args.bench_num}.{bench_name} contains {num_ckpt} ckpts.")
# exit(1)

result_dir = "/m5out_spec_dump_cache_2_to_1"
unit = 10000000
num_sample = 10
out_dir = os.getcwd() + '/out_entropy/'
log_dir = os.getcwd() + '/log_entropy/'
err_dir = os.getcwd() + '/err_entropy/'
create_folder(out_dir)
create_folder(err_dir)
create_folder(log_dir)

for scheme in ['entropy']:
    # check binary exists
    if not os.path.isfile(f"{scheme}"): 
        print(bcolors.RED + f"{scheme} binary file not exist!" + bcolors.ENDC)
        exit(1)
    num_snapshots=[]
    for xor_scheme in tqdm(["none", "rand", "ideal"], desc="xor scheme", leave=False):
        line = []
        bit = []
        for no_ckpt in tqdm(range(num_ckpt), desc="check point", leave=False):
            for i in range(num_sample):
                i = i + 1
                data_file = gem5_dir + result_dir + \
                    f"/{args.bench_num}/null/{no_ckpt}/{i*unit}"
                if not os.path.isdir(data_file): 
                    print(bcolors.RED + f"data dir not exist! {data_file}" + bcolors.ENDC)
                    continue
                cmd_list = [f"./{scheme}"] + [data_file] + [xor_scheme]
                p = subprocess.Popen(cmd_list, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                print(bcolors.OKGREEN + f'{bench_name} [{no_ckpt}.{i}] Launched:  ' + bcolors.ENDC + f"{scheme}+{xor_scheme}")
                # print(f'{p.args}')
                out,err = p.communicate()
                out = out.decode('utf-8')
                err = err.decode('utf-8')
                if err: 
                    print(bcolors.RED + err + bcolors.ENDC)
                    ferr = open(f"err/{args.bench_num}-{no_ckpt}.{i}.{scheme}_{xor_scheme}","w")
                    ferr.write(err)
                    ferr.write(f'{p.args}')
                    ferr.close()
                else:
                    if xor_scheme == 'none':
                        line_ = float(out.split('(')[-1].split(')')[0])
                        line.append(line_)
                        bit_ = float(out.split('{')[-1].split('}')[0])
                        bit.append(bit_)
                    elif xor_scheme == 'rand' or xor_scheme == 'ideal':
                        line_ = float(out.split('[')[-1].split(']')[0])
                        line.append(line_)
                        bit_ = float(out.split('|')[1])
                        bit.append(bit_)
                    else: assert False

            # exit(1)
        line_arith = sum(line)/len(line)
        line_geo = gmean(line)
        bit_arith = sum(bit)/len(bit)
        bit_geo = gmean(bit)
        num_snapshots.append(len(line))
        print(f"total {len(line)} snapshots.")

        # write means and #snapshots to output
        f = open(out_dir+f"/{bench_name}.{scheme}_{xor_scheme}", "w")
        f.write(f"{line_arith}, {line_geo}, {bit_arith}, {bit_geo}, {len(line)}")
        f.close()


    print(num_snapshots)
