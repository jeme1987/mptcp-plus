#!/usr/bin/env python3
import sys
import os
import re
import json
import pandas
import dirtyjson
import matplotlib.pyplot as plt
import numpy as np

def main(argv):
    with open(argv[1]) as file:
        d = dirtyjson.load(file)
        print('------ params -----\n')
        for k,v in d['params'].items():
            print('{}: {}'.format(k,v))

        print('------ app-statistics -----\n')
        stats = d['stats']
        pktLostRate = 1 - (stats['RxPktCnt']/stats['TxPktCnt'])
        print('TxPkts: {}, TxBytes: {}'.format(stats['TxPktCnt'], stats['TxPktCnt']*stats['PktSize']))
        print('RxPkts: {}, RxBytes: {}'.format(stats['RxPktCnt'], stats['RxPktCnt']*stats['PktSize']))
        print('PktLossRate: {}%'.format(pktLostRate))
        print('')

        print('latency:')
        df = pandas.DataFrame.from_dict(stats['Latency']).dropna()
        series = df['RxTime'] - df['TxTime']
        print(series.describe())

        fig, ax = plt.subplots()
        ax2 = ax.twinx()
        n, bins, patches = ax.hist(series, bins=100)
        n, bins, patches = ax2.hist(
            series, cumulative=1, histtype='step', bins=1000, density=1, color='tab:orange')

        ax.set_xlabel('Transmission Latency')
        ax2.set_ylabel('# of Packets')
        ax2.set_ylabel('CDF')

        x0, xmax = ax2.get_xlim()
        y0, ymax = ax2.get_xlim()
        data_width = xmax - x0
        data_height = ymax - y0
        ax2.text(x0 + data_width * 0.1, 0.8, 'mean: {:.2f} ms\n90%: {:.2f} ms\nPktLost: {:.2f}%'.format(series.mean()*1000, series.quantile(0.9)*1000, pktLostRate))
        plt.title('Youtube 4K @ UDP')
        plt.show()


        # df = pandas.DataFrame.from_dict(dirtyjson.load(file))W
        # print(df)
        # print(type(df))
        # df = pandas.read_json(cleanJson(file.))
        # print(df)

if __name__ == '__main__':
    main(sys.argv)