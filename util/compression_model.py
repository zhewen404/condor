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
    # parser = construct_argparser()
    # args = parser.parse_args()
    m = [2**n for n in range(9)] # segment number
    seg_size = [512 / m_ for m_ in m]
    seg_decode_bit = [math.log2(ss) for ss in seg_size]
    CE_num_1 = [math.floor(512 / (9-math.log2(m_)) - m_ + 1) for m_ in m]
    print(CE_num_1)
    CE_density = [CE1 / 512 * 100 for CE1 in CE_num_1]
    print(CE_density)

    # n = [1,2] # number of 1s actually in cache line
    n = list(range(1,512,1))
    compaction_ratio = [[512 / (9 - math.log2(m_)) / (m_- 1 + n_) for n_ in n ] for m_ in m]
    # print(compaction_ratio)
    
    fig = go.Figure()
    i = 0
    for comp_r in compaction_ratio:
        # print(comp_r)
        fig.add_trace(go.Scatter(x=n, y=comp_r,
                    mode='lines',
                    name=f'#segment={2**i}'))
        fig.add_trace(go.Scatter(x=[CE_num_1[i]], y=[512 / (9 - math.log2(2**i)) / (2**i - 1 + CE_num_1[i])],
                    mode='markers',
                    marker_symbol='star',
                    marker_color="lightskyblue",
                    marker_size=15,
                    name=f'CE point #segment={2**i}'
                    ))
        i += 1
        
    fig.add_trace(go.Scatter(x=n, y=[1]*len(n),
                mode='lines',
                name=f'baseline'))

    fig.update_layout(
        title="compaction ratio",
        xaxis_title="#1s in cache block",
        yaxis_title="compaction ratio",
        font=dict(
            family="Courier New, monospace",
            size=18,
            # color="RebeccaPurple"
        )
    )
    fig.show()