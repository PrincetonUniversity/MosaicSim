#!/usr/bin/env python

"""
    main.py
"""

from __future__ import print_function

import os
import sys
import argparse
import numpy as np
import pandas as pd
import networkx as nx
from tqdm import tqdm
from time import time
from collections import defaultdict

# --
# CLI

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--query-path', type=str, default='./_data/synthetic/queries/query.Subgraph.4.3.txt')
    parser.add_argument('--edge-path', type=str, default='./_data/synthetic/graphs/edges_small.tsv')
    parser.add_argument('--outdir', type=str, default='./_results/synthetic/small/')
    parser.add_argument('--sort', action="store_true")
    return parser.parse_args()


# --
# Helpers

def make_graph(edges):
    """ edgelist -> networkx graph """
    g = nx.Graph()
    for i, row in tqdm(edges.iterrows()):
        g.add_node(row.src, type=row.src_type)
        g.add_node(row.trg, type=row.trg_type)
        g.add_edge(row.src, row.trg, weight=row.weight)
    
    return g


def compute_signatures(g):
    """ compute number of neighbors of each type """
    for node in tqdm(g.nodes):
        counter = defaultdict(set)
        one_hop = set(g.neighbors(node))
        for neib in one_hop:
            
            node_type = g.nodes[neib]['type']
            
            # One hop
            counter[node_type].add(neib)
            
            # Two hops
            for neib2 in g.neighbors(neib):
                if (neib2 != node) and (neib2 not in one_hop):
                    node_type2 = g.nodes[neib2]['type']
                    counter[(node_type, node_type2)].add(neib2)
            
            g.nodes[node]['counter'] = dict([(k, len(v)) for k,v in counter.items()])
    
    return g


def compute_bounds(q):
    """ given a query, determine what topology thresholds we can use to prune graph """
    count_bound_types = {}
    for node in q.nodes:
        node_type = q.nodes[node]['type']
        node_counter = q.nodes[node]['counter']
        
        if not node_type in count_bound_types:
            count_bound_types[node_type] = set(node_counter.keys())
        else:
            count_bound_types[node_type] = count_bound_types[node_type].intersection(node_counter.keys())
    
    count_bounds = defaultdict(dict)
    for node in q.nodes:
        node_type = q.nodes[node]['type']
        node_counter = q.nodes[node]['counter']
        
        for count_type, count in node_counter.items():
            if count_type in count_bound_types[node_type]:
                if count_bounds[node_type].get(count_type, np.inf) > count:
                    count_bounds[node_type][count_type] = count
    
    return count_bounds


def prune_by_counts(g, edges, count_bounds):
    """ prune the reference graph using topology of query """
    g_nodes = list(g.nodes)
    for node in g_nodes:
        node_type = g.nodes[node]['type']
        node_counter = g.nodes[node]['counter']
        
        for count_type, count in count_bounds[node_type].items():
            if node_counter.get(count_type, -1) < count:
                g.remove_node(node)
                break
                
    sel = edges[['src', 'trg']].apply(lambda x: g.has_edge(x['src'], x['trg']), axis=1)
    edges = edges[sel].reset_index(drop=True)
    
    return g, edges


if __name__ == "__main__":
    
    args = parse_args()
    
    # --
    # Load query
    
    t = time()
    query_edgelist = pd.read_csv(args.query_path, sep='\t')
    q = compute_signatures(make_graph(query_edgelist))
    print('prep-query.py: loaded query %s in %fs' % (args.query_path, time() - t), file=sys.stderr)
    
    # --
    # Load graph (dropping edge types not in query)
    
    t = time()
    edgelist = pd.read_csv(args.edge_path, sep='\t')
    edgelist = edgelist[edgelist.edge_type.isin(query_edgelist.edge_type)].reset_index(drop=True)
    if args.sort:
        edgelist = edgelist.sort_values(['weight', 'edge_type'], ascending=[False, True])
    
    g = compute_signatures(make_graph(edgelist))
    print('prep-query.py: loaded edgelist %s in %fs' % (args.edge_path, time() - t), file=sys.stderr)
    
    # --
    # Prune graph using counts
    
    t = time()
    count_bounds = compute_bounds(q)
    r, edgelist = prune_by_counts(g, edgelist, count_bounds)
    print('prep-query.py: pruned graph in %fs' % (time() - t), file=sys.stderr)
    
    if not os.path.exists(args.outdir):
        os.makedirs(args.outdir)
    
    print('prep-query.py: writing to %s' % args.outdir, file=sys.stderr)
    query_edgelist.to_csv(os.path.join(args.outdir, 'query.tsv'), sep='\t', header=True, index=False)
    edgelist.to_csv(os.path.join(args.outdir, 'edgelist.tsv'), sep='\t', header=True, index=False)

