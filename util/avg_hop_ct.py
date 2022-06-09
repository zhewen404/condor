import argparse
import numpy as np
import plotly.express as px
import os
from utils import bcolors

def construct_argparser():
    parser = argparse.ArgumentParser(description='source and destination')
    parser.add_argument('-f',
                        '--file',
                        help='file to parse placement',
                        default='util/placement_dataFill.txt'
                        )
    return parser

def all_to_all(dim, min_degree=2):
    '''
    Assuming all to all traffic pattern, this function returns the avg hop 
    count for a mesh with dim and minimum degree.
    '''
    return min_degree*(dim/3-(1/(3*dim)))

if __name__ == "__main__":
    parser = construct_argparser()
    args = parser.parse_args()

    #//// placement initialization ////
    source = []
    destination = []
    with open(args.file, 'r') as searchfile:
        i = 0
        source_ = False
        dest_ = False
        for line in searchfile:
            if i == 0: dim = int(line)
            elif i == 1: routing = line.replace('\n', '')
            else: 
                if '=== source begin ===' in line: source_ = True
                elif '=== source end ===' in line: source_ = False
                elif '=== destination begin ===' in line: dest_ = True
                elif '=== destination end ===' in line: dest_ = False
                else:
                    tuple_0 = int(line.split(' ')[0])
                    tuple_1 = int(line.split(' ')[1])
                    if source_: 
                        source.append((tuple_0, tuple_1))
                    elif dest_:
                        destination.append((tuple_0, tuple_1))
            i += 1
    print(f'dim={dim}, routing={routing}')
    print(f'src={source}, {len(source)}')
    print(f'dst={destination}, {len(destination)}')
    # exit()
    # source=[(7,7)]
    # destination=[(6,7)]

    # //// routers initialization ////
    routers = []
    for i in range(dim):
        routers_ = []
        for j in range(dim):
            routers_.append(0)
        routers.append(routers_)
    
    # //// hop calculation ////
    hop = 0
    pair = 0
    for s in source:
        for d in destination:

            # determine routing
            if routing == 'X-Y':
                index = 0
            else:
                index = 1

            # first dimension traversal
            index2 = abs(index-1)
            first_diff = d[index] - s[index]
            second_diff = d[index2] - s[index2]
            if second_diff <0: startloc = -1
            else: startloc = 1
            # print(first_diff, second_diff)
            first_spacing = np.linspace(0, first_diff,abs(first_diff)+1).astype(int)
            second_spacing = np.linspace(startloc, second_diff,abs(second_diff)).astype(int)

            # print(first_spacing, s[index], d[index])
            # print(second_spacing, s[index2], d[index2])

            for i in first_spacing:
                if index == 0: 
                    # print(f'r[{s[0]+i}][{s[1]}] update')
                    routers[s[0]+i][s[1]] += 1
                    hop += 1
                else:
                    # print(f'r[{s[0]}][{s[1]+i}] update')
                    routers[s[0]][s[1]+i] += 1
                    hop += 1
            # print(f'first dim traversal done! routers={routers}, hop={hop}')

            # second dimension traversal
            for j in second_spacing:
                if index == 0: 
                    # print(f'r[{d[0]}][{s[1]+j}] update')
                    routers[d[0]][s[1]+j] += 1
                    hop += 1
                else:
                    # print(f'r[{s[0]+j}][{d[1]}] update')
                    routers[s[0]+j][d[1]] += 1
                    hop += 1
            # print(f'second dim traversal done! routers={routers}, hop={hop}')
            
            pair += 1
            hop -= 1
            # print(f'*** pair done, hop={hop}')
    avg_hop_ct = hop / pair
    print(bcolors.WARNING + f'total hop={hop}, pair={pair}, avg={avg_hop_ct}' + bcolors.ENDC)
    print(f'all-to-all hop={all_to_all(dim)}')

    fig = px.imshow(routers, text_auto=True)
    
    dir_ = "util/router/"
    if not os.path.exists(dir_): os.mkdir(dir_)
    placement_name = args.file.split('/')[-1].replace('.txt','')+'_'+routing
    name = dir_ + placement_name +'.png'
    fig.write_image(name)
    print(bcolors.OKGREEN+f'*** Written router util heatmap to {name}'+bcolors.ENDC)
