#!/usr/bin/env python
# coding: utf-8

# In[1]:


import plotly.express as px
import re
import plotly.io as pio
# pio.renderers.default = "browser"
pio.renderers.default = "notebook_connected"
from collections import OrderedDict
import argparse
from scipy.stats import gmean
import numpy as np
import plotly.graph_objects as go
import plotly.figure_factory as ff
import os
from util import bench_num_name_map


# In[2]:


tag='base_refrate_orig_config-m64.0000'


# In[3]:


bench_num=505
bench_name=bench_num_name_map[bench_num]


# In[4]:


file=f"/home/zhewen/repo/cpu2017/benchspec/CPU/{bench_num}.{bench_name}/run/run_{tag}/out.txt"


# In[5]:


find_comma_num = lambda line: int(re.findall(r'\b\d[\d,.]*\b', line)[1].replace(',',''))


# In[6]:


file1 = open(file, 'r')
Lines = file1.readlines()
  
count = 0
cycles = []
insns = []
begins = []
ends = []
# Strips the newline character
for line in Lines:
    count += 1
    if 'cpu_core/cycles/' in line: 
        cycle=find_comma_num(line)
        cycles.append(cycle)
#         print(cycles)
    elif 'cpu_core/instructions/' in line:
        insn=find_comma_num(line)
        insns.append(insn)
    elif 'probe_mcf_r_base:loop ' in line:
        begin=find_comma_num(line)
        begins.append(begin)
    elif 'probe_mcf_r_base:loop_end' in line:
        end=find_comma_num(line)
        ends.append(end)
        


# In[7]:


cpis = [c/i for c,i in zip(cycles,insns)]


# In[8]:


index_begin=[ i for i in range(len(begins)) if begins[i] == 1 ]
index_begin = index_begin[0:-1]#ignore last begin
index_end =[ i for i in range(len(ends)) if ends[i] == 1 ]
assert len(index_begin)==len(index_end)


# In[9]:


x=np.arange(len(cpis))
fig = go.Figure()

fig.add_trace(go.Scatter(x=x, y=cpis,
                mode='lines',
                name=f'cpi',
                ))
# fig.update_yaxes(type="log")
fig.update_layout(title=f'{bench_name} phase analysis',
                   xaxis_title='second',
                   yaxis_title='cpi')

for b,n in zip(index_begin, index_end):
    index = index_begin.index(b)
    if (index % 2) == 0: color = 'red' 
    else: color = "green"
    fig.add_vrect(x0=b, x1=n, line_width=0, fillcolor=color, opacity=0.1)
fig.show()


# In[ ]:




