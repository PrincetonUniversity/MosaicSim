#!/usr/bin/env python

"""
    main.py
"""

from __future__ import print_function

import sys
import json
import heapq
import argparse
import numpy as np
import pandas as pd
import networkx as nx
from time import time
from tqdm import tqdm
from collections import defaultdict

# --
# CLI

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--query-path', type=str, default='./_results/synthetic/small/query.tsv')
    parser.add_argument('--edge-path', type=str, default='./_results/synthetic/small/edgelist.tsv')
    parser.add_argument('--k', type=int, default=20)
    return parser.parse_args()

# --
# Helpers

def parse_row(row, sep='\t'):
    row = row.strip().split(sep)
    src, trg, weight, src_type, trg_type = int(row[0]), int(row[1]), float(row[2]), int(row[3]), int(row[4])
    return src, trg, weight, src_type, trg_type

class TopK(object):
    def __init__(self, k=20):
        self.k = k
        self.buffer = []
        self.counter = 0
        self.min = -np.inf
    
    def add(self, x):
        self.counter += 1
        
        if len(self.buffer) < self.k:
            heapq.heappush(self.buffer, (x['weight'], x))
        elif x['weight'] > self.buffer[0][0]:
            heapq.heapreplace(self.buffer, (x['weight'], x))
            self.min = self.buffer[0][0]


def initial_candidates(query, src, trg, weight, src_type, trg_type):
    for q_src, q_trg in query['all_edges']:
        if q_src < q_trg:
            q_src_type = query['types'][q_src]
            q_trg_type = query['types'][q_trg]
            
            if (q_src_type == src_type) and (q_trg_type == trg_type):
                yield {
                    "edges"  : [(q_src, q_trg), (q_trg, q_src)],
                    "nodes"  : {q_src : src, q_trg : trg},
                    "weight" : weight,
                    "num_uncovered_edges" : query['num_edges'] - 1,
                }
            
            if (q_src_type == trg_type) and (q_trg_type == src_type):
                yield {
                    "edges"  : [(q_src, q_trg), (q_trg, q_src)],
                    "nodes"  : {q_src : trg, q_trg : src},
                    "weight" : weight,
                    "num_uncovered_edges" : query['num_edges'] - 1,
                }


def expand_candidate(query, reference, top_k, cand, max_weight):
    # Find an uncovered edge w/ one covered endpoint
    for (q_src, q_trg) in query['all_edges']:
        if (q_src in cand['nodes']) and ((q_src, q_trg) not in cand['edges']):
            r_src = cand['nodes'][q_src]
            break
    
    new_edges, new_types = None, None
    
    # If covered node has neighbors of the right type
    q_trg_type  = query['types'][q_trg]
    r_src_neighbors = reference['neibs'][r_src]
    if q_trg_type in r_src_neighbors:
        
        q_trg_current = cand['nodes'].get(q_trg, None)
        cand_nodes_values = set(cand['nodes'].values())
        
        # For each neighbor of the right type
        for r_trg, r_edge_weight in r_src_neighbors[q_trg_type]:
            
            # If the target "fits"
            if ((q_trg_current is None) and (r_trg not in cand_nodes_values)) or (q_trg_current == r_trg):
                
                # If the edge isn't already used
                if (r_src, r_trg) not in reference['visited']:
                    
                    # If the upper bound is high enough
                    new_num_uncovered_edges = cand['num_uncovered_edges'] - 1
                    new_weight = cand['weight'] + r_edge_weight
                    
                    upper_bound = new_weight + max_weight * new_num_uncovered_edges
                    if upper_bound > top_k.min:
                        
                        if new_edges is None:
                            new_edges = cand['edges'] + [(q_src, q_trg), (q_trg, q_src)]
                        
                        new_nodes = cand['nodes'].copy()
                        new_nodes[q_trg] = r_trg
                            
                        new_cand = {
                            "edges"  : new_edges,
                            "nodes"  : new_nodes,
                            "weight" : new_weight,
                            "num_uncovered_edges" : new_num_uncovered_edges,
                        }
                        
                        if new_num_uncovered_edges == 0:
                            yield new_cand
                        else:
                            for c in expand_candidate(query, reference, top_k, new_cand, max_weight):
                                yield c


def cand2signature(cand, n):
    out = [None] * n
    for k,v in cand['nodes'].items():
        out[k] = v
    return tuple(out)


if __name__ == "__main__":
    
    args = parse_args()
    
    # --
    # Load query
    
    t = time()
    
    query_edges = pd.read_csv(args.query_path, sep='\t')
    
    query = {
        "all_edges" : set(
            [tuple(e) for e in query_edges[['src', 'trg']].values] +
            [tuple(e) for e in query_edges[['trg', 'src']].values]
        ),
        "types" : dict(
            [(node, node_type) for node, node_type in query_edges[['src', 'src_type']].values] +
            [(node, node_type) for node, node_type in query_edges[['trg', 'trg_type']].values]
        ),
        "num_edges" : query_edges.shape[0],
        "num_nodes" : len(set(list(query_edges.src) + list(query_edges.trg)))
    }
    
    print('main.py: loaded query %s in %fs' % (args.query_path, time() - t), file=sys.stderr)
    
    # --
    # Load graph
    
    t = time()
    
    reference = {
        "neibs" : defaultdict(lambda: defaultdict(list)),
        "visited" : set([])
    }
    
    for i, row in enumerate(open(args.edge_path)):
        if i == 0:
            continue
        else:
            src, trg, weight, src_type, trg_type = parse_row(row)
            reference['neibs'][src][trg_type].append((trg, weight))
            reference['neibs'][trg][src_type].append((src, weight))
    
    for k,v in reference['neibs'].items():
        reference['neibs'][k] = dict(v)
    
    reference['neibs'] = dict(reference['neibs'])
    
    print('main.py: loaded edges %s in %fs' % (args.edge_path, time() - t), file=sys.stderr)
    
    # --
    # Run
    
    print('-' * 100, file=sys.stderr)
    print('main.py: start search', file=sys.stderr)
    
    t0 = time()
    
    top_k = TopK(k=args.k)
    for i, row in tqdm(enumerate(open(args.edge_path))):
        if i == 0:
            continue
        
        src, trg, weight, src_type, trg_type = parse_row(row)
        
        if (weight * query['num_edges']) < top_k.min:
            break
        
        for initial_candidate in initial_candidates(query, src, trg, weight, src_type, trg_type):
            for candidate in expand_candidate(query, reference, top_k, initial_candidate, max_weight=weight):
                top_k.add(candidate)
            
            reference['visited'].add((src, trg))
            reference['visited'].add((trg, src))
    
    runtime = time() - t0
    
    # --
    # Check output and log
    
    candidate_signatures = [cand2signature(candidate, n=query['num_nodes']) for _, candidate in top_k.buffer]
    assert len(candidate_signatures) == len(set(candidate_signatures)), 'duplicate candidates!'
    
    for score, candidate in sorted(top_k.buffer, key=lambda x: x[0]):
        print(json.dumps({
            "match" : candidate['nodes'],
            "score" : score,
        }))
    
    print('-' * 100, file=sys.stderr)
    print('total_weight', sum([p[0] for p in top_k.buffer]), file=sys.stderr)
    print('runtime', runtime, file=sys.stderr)

