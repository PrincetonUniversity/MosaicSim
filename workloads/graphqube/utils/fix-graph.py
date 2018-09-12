#!/usr/bin/env python

"""
    fix-graph.py
    
    Go from UIUC format to something simpler
"""

from __future__ import print_function

import sys
import argparse
import numpy as np
import pandas as pd

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--graph-path', type=str, default='./_data/synthetic/graphs/GT_small.txt')
    parser.add_argument('--type-path', type=str, default='./_data/synthetic/graphs/types_small.txt')
    parser.add_argument('--outpath', type=str, default='./_data/synthetic/graphs/edges_small.tsv')
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    
    # --
    # IO
    
    print('prep.py: loading %s' % args.graph_path, file=sys.stderr)
    
    edges = pd.read_csv(args.graph_path, sep=' ', skiprows=8, header=None)
    edges = edges[[1,2,3]]
    edges.columns = ('src', 'trg', 'weight')
    
    edges.weight /= 100
    edges.weight = edges.weight.apply(lambda x: '%0.4f' % x)
    
    edges[['src', 'trg']] = edges[['src', 'trg']].apply(lambda x: sorted((x['src'], x['trg'])), axis=1)
    edges = edges.drop_duplicates(['src', 'trg'])
    
    # --
    # Add node labels
    
    print('prep.py: adding node types from' % args.type_file, file=sys.stderr)
    
    node_types = pd.read_csv(args.type_path, sep='\t', header=None)
    node_types.columns = ('node_id', 'type')
    
    edges = pd.merge(edges, node_types, left_on='src', right_on='node_id')
    edges.columns = ('src', 'trg', 'weight', 'delete_me_1', 'src_type')
    
    edges = pd.merge(edges, node_types, left_on='trg', right_on='node_id')
    edges.columns = ('src', 'trg', 'weight', 'delete_me_1', 'src_type', 'delete_me_2', 'trg_type')
    
    del edges['delete_me_1']
    del edges['delete_me_2']
    
    edges['edge_type'] = edges[['src_type', 'trg_type']].apply(lambda x: '%d#%d' % tuple(sorted((x['src_type'], x['trg_type']))), axis=1)
    
    edges['edge_type'] = edges.src_type.astype(str) + '#' + edges.trg_type.astype(str)
    sel = edges.trg_type < edges.src_type
    edges.edge_type[sel] = edges.trg_type[sel].astype(str) + '#' + edges.src_type[sel].astype(str)
    
    # --
    # Write to disk
    
    edges = edges.sort_values('weight', ascending=False).reset_index(drop=True)
    print('prep.py: writing to %s' % args.outpath, file=sys.stderr)
    edges.to_csv(args.outpath, sep='\t', index=False)
