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
# ckpt_dir=$(find ${ckpt} -mindepth 1 -maxdepth 1 -type d -name "${spec_bench_num}*")
# echo "ckpt dir is ${ckpt_dir}"
# num_simpoint=$(find ${ckpt_dir} -mindepth 1 -maxdepth 1 -type d | wc -l)
# echo "number of simpoints: ${num_simpoint}"
from glob import glob
num_ckpt = len(glob(os.path.join(ckpt, "cpt.*", "")))
print(f"spec {args.bench_num}.{bench_name} contains {num_ckpt} ckpts.")
# exit(1)

result_dir = "/m5out_spec_dump_cache"
unit = 10000000
num_sample = 10
log_dir = os.getcwd() + '/out/'
err_dir = os.getcwd() + '/err/'
create_folder(log_dir)
create_folder(err_dir)

for scheme in tqdm(["bdi", "dedup"], desc="compression scheme", leave=True):
    # check binary exists
    if not os.path.isfile(f"{scheme}"): 
        print(bcolors.RED + f"{scheme} binary file not exist!" + bcolors.ENDC)
        exit(1)
    num_snapshots=[]
    for xor_scheme in tqdm(["none", "rand", "ideal"], desc="xor scheme", leave=False):
        cr = []
        for no_ckpt in tqdm(range(num_ckpt), desc="check point", leave=False):
            for i in range(num_sample):
                i = i + 1
                data_file = gem5_dir + result_dir + \
                    f"/{args.bench_num}/null/{no_ckpt}/{i*unit}"
                if not os.path.isdir(data_file): 
                    print(bcolors.RED + f"data dir not exist! {data_file}" + bcolors.ENDC)
                    continue
                cmd_list = [f"./{scheme}"] + [data_file] + [xor_scheme]
                # with open(f"err/{args.bench_num}-{no_ckpt}.{i}.{scheme}_{xor_scheme}","wb") as err:
                # with open(f"log/{args.bench_num}-{i}.log","wb") as out, open(f"err/{args.bench_num}-{i}.err","wb") as err:
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
                    cr_ = float(out.split('(')[-1].split(')')[0])
                    print(cr_)
                    cr.append(cr_)
            # exit(1)
        cr_avg = sum(cr)/len(cr)
        num_snapshots.append(len(cr))
        print(f"total {len(cr)} snapshots.")
        f = open(log_dir+f"/{bench_name}.{scheme}_{xor_scheme}", "w")
        f.write(f"{cr_avg}, {len(cr)}")
        f.close()
    print(num_snapshots)