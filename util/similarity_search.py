import argparse
import numpy as np
import plotly.express as px
import os
from utils import bcolors
import math
import plotly.graph_objects as go
import random
from scipy.spatial.distance import hamming
from itertools import combinations
from tqdm import tqdm
from iteration_utilities import random_combination
from sklearn.metrics.pairwise import cosine_similarity

import plotly.io as pio
pio.renderers.default = "browser"

def hash(array, a):
    out = np.dot(array, a)
    out[np.where(out>0)] = 1
    out[np.where(out<=0)] = 0
    return out

def toDec(out):
    '''convert list of 1 and 0 to decimal int'''
    out_ = 0
    for bit in out: out_ = (out_ << 1) | bit
    return out_

def toBin_seg(a, nbit):
    '''convert a decimal to a list of 0 and 1 padded with 0 if needed'''
    a = list(format(a, f'0{nbit}b'))
    a = list(map(int, a))
    return a

def toBin(base, segment_width):
    '''convert a list of decimal to a flattened list of 1 and 0,
        individual decimal is padded if needed.
    '''
    base_bin_arr = [toBin_seg(a, segment_width) for a in base]
    base_bin = [item for sublist in base_bin_arr for item in sublist]
    return base_bin
    # print(base_bin)
    # assert len(base_bin) == line_width

def toSeg(binary, segment_width):
    '''chunk a list of 0 and 1 according to segment_width and convert
        each segment into decimal.
    '''
    nseg = int(len(binary)/segment_width)
    # print(nseg)
    binary_group = np.array(binary).reshape(nseg, segment_width)
    # print(binary_group[0])
    # print(len(binary_group))
    ret_dec_list = []
    for i in range(len(binary_group)):
        decimal = toDec(binary_group[i])
        # print(decimal)
        ret_dec_list.append(decimal)
    return ret_dec_list

def construct_argparser():
    parser = argparse.ArgumentParser(description='param')
    parser.add_argument('-s',
                        '--seg',
                        help='number of segment',
                        type=int,
                        required=False,
                        default=64,
                        )
    parser.add_argument('-p',
                        '--hyp',
                        help='number of hyperplanes',
                        type=int,
                        required=False,
                        default=10,
                        )
    parser.add_argument('-n',
                        '--sample',
                        help='number of samples',
                        type=int,
                        required=False,
                        default=256,
                        )
    parser.add_argument(
                        '--sparse',
                        help='sparsity level',
                        type=int,
                        required=False,
                        default=6,
                        )
    parser.add_argument(
                        '--line',
                        help='line size',
                        type=int,
                        required=False,
                        default=512,
                        )
    return parser

if __name__ == "__main__":
    parser = construct_argparser()
    args = parser.parse_args()

    line_width = args.line
    nsegment = args.seg #segment
    segment_width = int(line_width / nsegment)
    nhyperplanes = args.hyp # sets bits
    sample_size = args.sample

    # construct LSH matrix
    n = args.sparse
    np.random.seed(2)
    array = np.random.randint(n, size=(nhyperplanes, nsegment))
    array[np.where(array == 0)] = -1
    for l in range(1,n-1):
        # print(l)
        array[np.where((array == l))] = 0
    # exit()
    # array[np.where(array == 2)] = 0
    # array[np.where(array == 3)] = 0
    # array[np.where(array == 4)] = 0
    array[np.where(array == (n-1))] = 1
    # print(array[0])
    
    random.seed(30)

    # # =========== testing ==============
    # a = random.randint(0, 2**nsegment-1)
    # print('a=',a)
    # a = toBin_seg(a, nsegment)
    # a_out = hash(array, a)
    # print('aout=',a_out)

    # b = [0]*100 + [1]*1 + [1]*11 + [0]*400
    # b_out = hash(array, b)
    # print('b=',toDec(b))
    # print('bout=', b_out)

    # c = [0]*100 + [0]*1 + [1]*11 + [0]*400
    # c_out = hash(array, c)
    # print('c=',toDec(c))
    # print('cout=', c_out)
    # # =========== testing ==============

    base = np.random.randint(2, size=line_width)
    base = toSeg(base, segment_width)
    # base = [5]*16 + [10]*8 + [1]*4 + [15]*4 + [32]*32
    # print(base)
    # exit()
    base_out = hash(array, base)
    # print('base =',toDec(base))
    print('base =', base)
    print('base_out =', base_out)

    base_bin = toBin(base, segment_width)
    # print(base_bin)

    # producing hamming distance of 1 from base
    fig = go.Figure()

    # dist = range(line_width)
    dist = list(range(1,line_width))
    colision_rate = []
    # cos_arr = []

    # for dist_ in dist:
    for k in tqdm(range(len(dist))):
        dist_ = dist[k]
        iter = []
        # iter = list(combinations(range(512), dist_))
        # iter = random.sample(iter, min(sample_size, len(iter)))
        while len(iter) < sample_size:
            rc = tuple(random_combination(range(line_width), r=dist_))
            if rc not in iter: iter.append(rc)
        # print(len(iter))
        # print(iter)
        # exit()
        ham_hash = []
        ham_line_bit = []
        ham_line_byte = []
        l1_line_byte = []

        for j in range(len(iter)):
            i = iter[j]
            # print(i)
            # toggle bits in original line
            base_ham_1 = base_bin[:]
            for index in i:
                base_ham_1[index] = base_bin[index] ^ 1
                # cos = cosine_similarity([base_bin], [base_ham_1])
                # cos_arr.append(cos)
            # print(base_ham_1, base_bin)

            # hamming distance of bit stream
            ham_line_bit_ = hamming(base_ham_1, base_bin) * len(base_bin)
            ham_line_bit.append(ham_line_bit_)

            base_ham_1_seg = toSeg(base_ham_1, segment_width)

            # hamming distance of byte stream
            ham_line_byte_ = hamming(base_ham_1_seg, base) * len(base)
            ham_line_byte.append(ham_line_byte_)
            # print(base_ham_1_seg, base)
            # print("ham of byte stream=", ham_line_byte_)
            # print('base_1=', base_ham_1_seg)

            # l1 distance of byte stream
            l1_line_byte_ = np.linalg.norm((base_ham_1_seg, base), ord=1)
            l1_line_byte.append(l1_line_byte_)
            # print("l1 of byte stream=", l1_line_byte_)

            # LSH hashing
            base_ham_1_out = hash(array, base_ham_1_seg)
            # print('base1=',toDec(base_ham_1))
            # print('base1_out=', base_ham_1_out)

            # hamming distance of hash value (finger print)
            ham = hamming(base_ham_1_out, base_out) * len(base_out)
            ham_hash.append(ham)
            # print('ham of hash val=', ham)

            # print()

        ########### ploting ###############
        # for i in range(line_width):
        # # plot data point scatter
        # fig.add_trace(go.Scatter(
        #     # x=l1_line_byte, 
        #     x=ham_line_bit,
        #     y=ham_hash,
        #     mode='markers',
        #     marker_symbol='circle',
        #     # marker_color="lightskyblue",
        #     opacity=0.5,
        #     marker_size=10,
        #     name=f'hamming dist of line val (bitstream) = 1.0'
        #     ))
        # # plot box plot
        # fig.add_trace(go.Box(
        #     y=ham_hash,
        #     x=ham_line_bit,
        #     name=f'hamming dist of line val={dist_} bit'
        #     # marker_color='#3D9970'
        # ))

        # print summary
        print(f'dist={dist_}: total {len(ham_hash)} points profiled.')
        ham_hash_avg = sum(ham_hash) / len(ham_hash)
        ham_line_bit_avg = sum(ham_line_bit) / len(ham_line_bit)
        ham_line_byte_avg = sum(ham_line_byte) / len(ham_line_byte)
        l1_line_byte_avg = sum(l1_line_byte) / len(l1_line_byte)
        all0 = [ i for i in range(len(ham_hash)) if ham_hash[i] == 0 ]
        colision_rate_ = len(all0) / len(ham_hash) * 100
        colision_rate.append(colision_rate_)
        print('hamming dist of line val (bitstream) =', ham_line_bit_avg)
        print('hamming dist of line val (bytestream) =', ham_line_byte_avg)
        print('l1 dist of line val      (bytestream) =', l1_line_byte_avg)
        
        print('hamming dist of hash val=', ham_hash_avg)
        print(f'LSH colision rate={colision_rate_}%')
        print()

    print('plotting...')
    # plot collision rate line
    fig.add_trace(go.Scatter(
        # x=l1_line_byte, 
        x=dist,
        y=colision_rate,
        mode='lines',
        marker_symbol='circle',
        # marker_color="lightskyblue",
        marker_size=10,
        name=f'hamming dist of line val={dist_} bit'
        ))

    fig.update_layout(
        title=f"LSH ({nsegment} segment, {nhyperplanes} hyperplane, {args.sparse} sparsity)\
            <br><sup>{sample_size} samples per distance profiled</sup>",
            # xaxis_title="value dist",
            yaxis_title="collision rate",
            # yaxis_title="hamming dist of hash val",
            xaxis_title="hamming dist of line val (bit)",
        font=dict(
            family="Courier New, monospace",
            size=18,
            # color="RebeccaPurple"
        )
    )
    fig.update(layout_yaxis_range = [0,100])
    fig.show()

    if not os.path.exists("lsh"):
        os.mkdir("lsh")
    fig.write_image(f"lsh/seg{nsegment}_hyp{nhyperplanes}_sparse{args.sparse}_sam{sample_size}_l{args.line}.png")

    
    # cos_mean = np.mean(cos_arr)
    # print('avg cos sim=', cos_mean)