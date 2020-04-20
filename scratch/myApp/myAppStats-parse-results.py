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

TITLE = 'Youtube 4K @ UDP'

def genTrafficPatternGraph(df, pktSize, interval_sec):

    # Find the start time and end time
    start_time = math.floor(df['TxTime'].min())
    end_time = max(math.ceil(df['RxTime'].max()), math.ceil(df['TxTime'].max()))

    # Generate fixed time slots between start time and end time
    d = {'Time': [], 'TxBytes':None, 'RxBytes':None}
    t = start_time
    while t <= end_time:
        d['Time'].append(t)
        t += interval_sec
    d['TxBytes'] = [0]*len(d['Time'])
    d['RxBytes'] = [0]*len(d['Time'])

    # Calculate Tx/Rx bytes from DataFrame
    for idx, row in df.iterrows(): 
        d['TxBytes'][math.floor((row['TxTime'] - start_time)/0.1)] += pktSize
        d['RxBytes'][math.floor((row['RxTime'] - start_time)/0.1)] += pktSize

    # Generate new DataFrame 
    new_df = pd.DataFrame.from_dict(d)
    new_df['Sent'] = new_df['TxBytes'] / 1024
    new_df['Received'] = new_df['RxBytes'] / 1024

    # Plot the image
    fig = plt.figure()
    ax = plt.gca()
    new_df.plot(kind='line',x='Time',y='Sent',ax=ax)
    new_df.plot(kind='line',x='Time',y='Received', color='red', ax=ax)
    ax.set_xlabel('Simulation time (second)')
    ax.set_ylabel('KBytes')

    title = 'Simulation Traffic Pattern\n{}'.format(TITLE)
    plt.title(title)

    print('save {}.png ...'.format(title))
    fig.savefig('{}.png'.format(title))


def genLetencyCDF(df, pktLostRate):
    fig, ax = plt.subplots()
    ax2 = ax.twinx()
    n, bins, patches = ax.hist(df['latency'], bins=100)
    n, bins, patches = ax2.hist(
        df['latency'], cumulative=1, histtype='step', bins=1000, density=1, color='tab:orange')

    ax.set_xlabel('Transmission Latency')
    ax2.set_ylabel('# of Packets')
    ax2.set_ylabel('CDF')

    x0, xmax = ax2.get_xlim()
    y0, ymax = ax2.get_xlim()
    data_width = xmax - x0
    data_height = ymax - y0
    ax2.text(x0 + data_width * 0.1, 0.8, 'mean: {:.2f} ms\n90%: {:.2f} ms\nPktLost: {:.2f}%'.format(df['latency'].mean()*1000, df['latency'].quantile(0.9)*1000, pktLostRate))
    
    title = 'Simulation Latecny Distribution\n{}'.format(TITLE)
    plt.title(title)

    print('save {}.png ...'.format(title))
    fig.savefig('{}.png'.format(title))


def main(argv):
    with open(argv[1]) as file:

        # Read data from output CSV
        d = dirtyjson.load(file)

        # Dump parameters
        print('------ params -----\n')
        for k,v in d['params'].items():
            print('{}: {}'.format(k,v))

        # Show statistics results
        stats = d['stats']
        pktLostRate = (1 - (stats['RxPktCnt']/stats['TxPktCnt']))*100
        print('------ app-statistics -----\n')
        print('TxPkts: {}, TxBytes: {}'.format(stats['TxPktCnt'], stats['TxPktCnt']*stats['PktSize']))
        print('RxPkts: {}, RxBytes: {}'.format(stats['RxPktCnt'], stats['RxPktCnt']*stats['PktSize']))
        print('PktLossRate: {}%'.format(pktLostRate))
        print('')

        # Calculate and describe latency
        df = pd.DataFrame.from_dict(stats['Latency']).dropna()
        df['latency'] = df['RxTime'] - df['TxTime']
        print('latency:')
        print(df['latency'].describe())
        
        # Output traffic pattern graph 
        genTrafficPatternGraph(df, stats['PktSize'], 0.1)

        # Output CDF of latency
        genLetencyCDF(df, pktLostRate)

        plt.show()

if __name__ == '__main__':
    main(sys.argv)