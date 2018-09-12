#!/usr/bin/env python

"""
    fix-queries.py
"""

from __future__ import print_function

import os
import sys
import argparse
import numpy as np
import pandas as pd
from glob import glob
from tqdm import tqdm

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--indir', type=str, default='../_data/synthetic/queries')
    return parser.parse_args()

def load_query(query_path, query_type_path):
    query_edges = pd.read_csv(query_path, sep='#', skiprows=5, header=None)
    query_edges.columns = ('src', 'trg', 'weight')
    query_edges = query_edges[query_edges.src <= query_edges.trg].reset_index(drop=True)
    query_edges.src -= 1   # zero based indexing
    query_edges.trg -= 1   # zero based indexing
    query_edges.weight = 0
    
    query_types = pd.read_csv(query_type_path, sep='\t', header=None)
    query_types.columns = ('node_id', 'type')
    query_type_lookup = pd.Series(np.array(query_types.type).astype(int), index=np.array(query_types.node_id) - 1).to_dict()
    
    query_edges['src_type']  = query_edges.src.apply(lambda x: query_type_lookup[x])
    query_edges['trg_type']  = query_edges.trg.apply(lambda x: query_type_lookup[x])
    query_edges['edge_type'] = query_edges[['src_type', 'trg_type']].apply(lambda x: '%d#%d' % tuple(sorted((x['src_type'], x['trg_type']))), axis=1)
    
    return query_edges

if __name__ == "__main__":
    args = parse_args()
    
    for query_path in tqdm(glob(os.path.join(args.indir, 'queryGraph*txt'))):
        
        query_type_path = os.path.basename(query_path).split('.')
        query_type_path[0] = 'queryTypes'
        query_type_path = os.path.join(os.path.dirname(query_path), '.'.join(query_type_path))
        
        outpath = os.path.basename(query_path).split('.')
        outpath[0] = 'query'
        outpath = os.path.join(os.path.dirname(query_path), '.'.join(outpath))
        
        query_edges = load_query(query_path, query_type_path)
        
        query_edges.to_csv(outpath, sep='\t', index=False)