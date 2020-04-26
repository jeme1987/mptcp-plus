#!/usr/bin/env python3
import sys
import os
import re
import json
import pandas as pd
import dirtyjson
import matplotlib.pyplot as plt
import numpy as np
import math



def parseLatency(filepath):
    # Read data from output CSV
    with open(filepath) as file:
        d = dirtyjson.load(file)
        df = pd.DataFrame.from_dict(d['stats']['Latency']).dropna()
        df['latency'] = df['RxTime'] - df['TxTime']
        return df['latency'], (1 - (d['stats']['RxPktCnt']/d['stats']['TxPktCnt']))*100


# TITLE = 'Latency Distribution\nZoom-like Traffic Pattern'
TITLE = 'Latency Distribution\nYoutube-like Traffic Pattern'
DIR = 'scratch/myApp/results'
PROTOCOLS = [
    {
        'name': 'UDP',
        'path': None,
        'color': 'tab:red'
    },
    {
        'name': 'TCP',
        'path': None,
        'color': 'purple'
    },
    {
        'name': 'Redundant',
        'path': None,
        'color': 'navy'
    },
    {
        'name': 'FastRTT',
        'path': None,
        'color': 'darkred'
    },
    {
        'name': 'RoundRobin',
        'path': None,
        'color': 'green'
    },
    {
        'name': 'OTIAS',
        'path': None,
        'color': 'gold'
    }
]
for x in os.listdir(DIR):
    for protocol in PROTOCOLS:
        if protocol['name'] == x:
            # protocol['path'] = '{}/{}/Itv0.2_pkt5/myApp.appStats.json'.format(DIR,x)
            protocol['path'] = '{}/{}/Itv2.0_pkt1000/myApp.appStats.json'.format(DIR,x)
            if not os.path.exists(protocol['path']):
                protocol['path'] = None

plt.xlabel('Transmission Latency')
plt.ylabel('CDF')

legend_str = []
for protocol in PROTOCOLS:
    if protocol['path'] == None:
        continue
    series, pktLost = parseLatency(protocol['path'])
    # series[len(series)] = 1.0060000000000002 # Force alignment
    series[len(series)] = 2.8690000000000007
    n, bins, patches = plt.hist(
        series, cumulative=1, histtype='step', bins=1000, density=1, color=protocol['color'],
        label='{}'.format(protocol['name']))
    
plt.legend(loc='lower right', fontsize='x-large')
plt.grid(True)
plt.title(TITLE)
plt.show()



