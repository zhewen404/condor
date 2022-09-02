import plotly.express as px
import re
import plotly.io as pio
# pio.renderers.default = "browser"
from collections import OrderedDict
import argparse
from scipy.stats import gmean
import numpy as np
import plotly.graph_objects as go
import plotly.figure_factory as ff

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def get_list(file, metric):
    pattern = pattern_map[metric]

    list_ = []
    with open(file, 'r') as searchfile:
        for line in searchfile:
            if re.match(pattern, line):
                prune_func = prune_map[metric]
                res = prune_func(line)
                list_.append(res)
    # print(list_)
    return list_

def to_matrix(l, n):
    return [l[i:i+n] for i in range(0, len(l), n)]

pattern_map = {
    'hostseconds': 'hostSeconds\s+',
    # stack distance
    'l1w': 'system.ruby.l1_cntrl\d+.cache.writeLinearHist::0-31\s+',
    'l1r': 'system.ruby.l1_cntrl\d+.cache.readLinearHist::0-31\s+',
    'l2r': 'system.ruby.l2_cntrl\d+.L2cache.readLinearHist::0-31\s+',
    'l2w': 'system.ruby.l2_cntrl\d+.L2cache.writeLinearHist::0-31\s+',
    'l2_t': 'system.ruby.l2_cntrl\d+.L2cache.m_demand_accesses\s+\d+',
    'l2_inf': 'system.ruby.l2_cntrl\d+.L2cache.infiniteSD\s+\d+',
    'l1_t': 'system.ruby.l1_cntrl\d+.cache.m_demand_misses\s+\d+',
    'l1_inf': 'system.ruby.l1_cntrl\d+.cache.infiniteSD\s+\d+',
    'l1w1': 'system.ruby.l1_cntrl\d+.cache.writeLinearHist::32-63\s+',
    'l1r1': 'system.ruby.l1_cntrl\d+.cache.readLinearHist::32-63\s+',
    'l2r1': 'system.ruby.l2_cntrl\d+.L2cache.readLinearHist::32-63\s+',
    'l2w1': 'system.ruby.l2_cntrl\d+.L2cache.writeLinearHist::32-63\s+',
    
    # cache util
    'l2_access': 'system.ruby.l2_cntrl\d+.L2cache.m_demand_accesses\s+\d+',
    'l1_access': 'system.ruby.l1_cntrl\d+.cache.m_demand_accesses\s+\d+',
    'l2_miss': 'system.ruby.l2_cntrl\d+.L2cache.m_demand_misses\s+\d+',
    'l1_miss': 'system.ruby.l1_cntrl\d+.cache.m_demand_misses\s+\d+',
    
    # cache capacity util
    'l1_ua': 'system.ruby.l1_cntrl\d+.cache.m_unique_access_ct::\d+-\d+',
    'l2_ua': 'system.ruby.l2_cntrl\d+.L2cache.m_unique_access_ct::\d+-\d+',
    
}

prune_map = {
    'hostseconds': lambda line: float(re.findall("\d+\.\d+", line)[0]),
    # stack distance
    'l2r': lambda line: int(re.sub('system.ruby.l2_cntrl\d+.L2cache.readLinearHist::0-31\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l2w': lambda line: int(re.sub('system.ruby.l2_cntrl\d+.L2cache.writeLinearHist::0-31\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l1r': lambda line: int(re.sub('system.ruby.l1_cntrl\d+.cache.readLinearHist::0-31\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l1w': lambda line: int(re.sub('system.ruby.l1_cntrl\d+.cache.writeLinearHist::0-31\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l2_t': lambda line: int(line.replace(' ','').split('demand_accesses')[-1].split('#')[0]),
    'l2_inf': lambda line: int(line.replace(' ','').split('infiniteSD')[-1].split('#')[0].split('(')[0]),
    'l1_t': lambda line: int(line.replace(' ','').split('demand_misses')[-1].split('#')[0]),
    'l1_inf': lambda line: int(line.replace(' ','').split('infiniteSD')[-1].split('#')[0].split('(')[0]),
    'l2r1': lambda line: int(re.sub('system.ruby.l2_cntrl\d+.L2cache.readLinearHist::32-63\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l2w1': lambda line: int(re.sub('system.ruby.l2_cntrl\d+.L2cache.writeLinearHist::32-63\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l1r1': lambda line: int(re.sub('system.ruby.l1_cntrl\d+.cache.readLinearHist::32-63\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    'l1w1': lambda line: int(re.sub('system.ruby.l1_cntrl\d+.cache.writeLinearHist::32-63\s+', '',\
                                   re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line)).replace(' ','').split('#')[0]),
    # cache util
    'l2_access': lambda line: int(line.replace(' ','').split('demand_accesses')[-1].split('#')[0]),
    'l1_access': lambda line: int(line.replace(' ','').split('demand_accesses')[-1].split('#')[0]),
    'l2_miss': lambda line: int(line.replace(' ','').split('demand_misses')[-1].split('#')[0]),
    'l1_miss': lambda line: int(line.replace(' ','').split('demand_misses')[-1].split('#')[0]),
    
    # cache capacity util
    'l1_ua': lambda line: tuple([int(str_) for str_ in re.search(r"system.ruby.l1_cntrl\d+.cache.m_unique_access_ct::(\d+)-\d+\s+(\d+)\s+", \
                                          re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line).split('#')[0]).groups()]),
    'l2_ua': lambda line: tuple([int(str_) for str_ in re.search(r"system.ruby.l2_cntrl\d+.L2cache.m_unique_access_ct::(\d+)-\d+\s+(\d+)\s+", \
                                          re.sub('[+-]?([0-9]*[.])?[0-9]+%', '', line).split('#')[0]).groups()]),
}

spec17_working_set_map = {
    'H': ['mcf', 'cactuBSSN', 'lbm'],
    'M': ['omnetpp', 'xz 1', 'xz 2', 'xz 3', \
            'bwaves 1', 'bwaves 2', 'bwaves 3', 'bwaves 4', \
            'cam4', 'fotonik3d', 'roms', \
        ],
    'L': ['deepsjeng', 'leela', 'exchange2', 'namd', \
            'x264 1', 'x264 3', \
            'povray', 'wrf', 'blender', 'imagick', 'nab', \
        ],
}

spec17_bench_map = {
    1: ['povray'],
    2: ['cam4'],
    3: ['nab'],
    4: ['mcf'],
    5: ['namd'],
    6: ['omnetpp'],
    7: ['lbm'],
    8: ['fotonik3d'],
    9:  spec17_working_set_map['H'], #h
    10: ['povray', 'cam4', 'nab', 'mcf', 'namd', \
        'omnetpp', 'lbm', 'fotonik3d'], #hetro
    11:  spec17_working_set_map['L'],#L
    12: ['omnetpp', 'cam4', 'fotonik3d', 'roms', 'xz 1', 'xz 2', 'xz 3'],#M
    13: ['cactuBSSN'],
    14: ['xz 1'],
    15: ['xz 2'],
    16: ['xz 3'],
    17: ['roms'],
    18: ['deepsjeng'],
    19: ['leela'],
    20: ['exchange2'],
    21: ['x264 1'],
    22: ['x264 3'],
    23: ['wrf'],
    24: ['blender'],
    25: ['imagick'],
}
