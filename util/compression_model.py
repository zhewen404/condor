import argparse
import numpy as np
import plotly.express as px
import os
from utils import bcolors
import math
import plotly.graph_objects as go

import plotly.io as pio
pio.renderers.default = "browser"

# def construct_argparser():
#     parser = argparse.ArgumentParser(description='source and destination')
#     parser.add_argument('-f',
#                         '--file',
#                         help='file to parse placement',
#                         default='util/placement_dataFill.txt'
#                         )
#     return parser
if __name__ == "__main__":
    LINESIZE = 512
    # parser = construct_argparser()
    # args = parser.parse_args()
    m = [2**n for n in range(9)] # segment number
    seg_size = [LINESIZE / m_ for m_ in m]
    seg_decode_bit = [math.log2(ss) for ss in seg_size]

    get_num_1s_at_CE_sparse = lambda LINESIZE, num_seg: math.floor(LINESIZE / (9-math.log2(num_seg)) - num_seg + 1)
    CE_num_1 = [get_num_1s_at_CE_sparse(LINESIZE, m_) for m_ in m]
    print(CE_num_1)

    CE_density = [CE1 / LINESIZE * 100 for CE1 in CE_num_1]
    print(CE_density)

    n = list(range(1,LINESIZE,1))# number of 1s actually in cache line
    get_compaction_ratio_sparse = lambda LINESIZE, num_seg, num_1s: LINESIZE / (9 - math.log2(num_seg)) / (num_seg- 1 + num_1s)
    compaction_ratio = [[get_compaction_ratio_sparse(LINESIZE, m_, n_) for n_ in n ] for m_ in m]

    # leading 0 encoding: value similarity aware
    get_num_bits_after_leading0comp = lambda LINESIZE, numbits_diff_from_LSB: math.log2(LINESIZE) + numbits_diff_from_LSB
    num_bits_after_comp = [get_num_bits_after_leading0comp(LINESIZE, n_) for n_ in n]
    compaction_ratio_leading0comp = [LINESIZE / i for i in num_bits_after_comp]

    # 1 segmented encoding:
    get_num_bits_after_1segcomp = lambda LINESIZE, num_1s: (num_1s + 1) * math.log2(LINESIZE)
    num_bits_after_1segcomp = [get_num_bits_after_1segcomp(LINESIZE, n_) for n_ in n]
    compaction_ratio_1segcomp = [LINESIZE / i for i in num_bits_after_1segcomp]


    ########### ploting ###############
    fig = go.Figure()
    i = 0
    # plot sparse segment
    for comp_r in compaction_ratio:
        fig.add_trace(go.Scatter(x=n, y=comp_r,
                    mode='lines',
                    name=f'#segment={2**i}'))
        fig.add_trace(go.Scatter(
            x=[CE_num_1[i]], 
            y=[LINESIZE / (9 - math.log2(2**i)) / (2**i - 1 + CE_num_1[i])],
                    mode='markers',
                    marker_symbol='star',
                    marker_color="lightskyblue",
                    marker_size=15,
                    name=f'CE point #segment={2**i}'
                    ))
        i += 1

    #plot leading 0 mode
    fig.add_trace(go.Scatter(x=n, y=compaction_ratio_leading0comp,
                mode='lines',
                name=f'leading 0'))
    
    # plot 1 segmented mode
    fig.add_trace(go.Scatter(x=n, y=compaction_ratio_1segcomp,
                mode='lines',
                name=f'1 segmented'))
    
    # plot baseline of 1
    fig.add_trace(go.Scatter(x=n, y=[1]*len(n),
                mode='lines',
                name=f'baseline'))

    fig.update_layout(
        title="compaction ratio",
        xaxis_title="#1s in cache block OR #bits different from LSB",
        yaxis_title="compaction ratio",
        font=dict(
            family="Courier New, monospace",
            size=18,
            # color="RebeccaPurple"
        )
    )
    fig.show()