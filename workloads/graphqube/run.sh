#!/bin/bash

# run.sh

# --------------------------------------------------------------------------------
# Run on small synthetic graphs (up to ~20 seconds, depending on query)

mkdir -p {_data,_results}
wget --header "Authorization:$TOKEN" https://hiveprogram.com/data/_v1/graphqube/synthetic.tar.gz
tar -xzvf synthetic.tar.gz && rm synthetic.tar.gz
mv synthetic _data/synthetic

# Prep graph and query
SIZE="medium"
QUERY="Subgraph.4.5"
python prep-query.py \
    --query-path ./_data/synthetic/queries/query.$QUERY.txt \
    --edge-path ./_data/synthetic/graphs/edges_$SIZE.tsv \
    --outdir ./_results/synthetic/$SIZE

# Run query (Python)
time python main.py \
    --query-path ./_results/synthetic/$SIZE/query.tsv \
    --edge-path ./_results/synthetic/$SIZE/edgelist.tsv

# Run query (C++)
time ./cpp/main \
    ./_results/synthetic/$SIZE/query.tsv \
    ./_results/synthetic/$SIZE/edgelist.tsv \
    20

# --------------------------------------------------------------------------------
# Run on wikipedia graph (< 1 second)

mkdir -p {_data,_results}
wget --header "Authorization:$TOKEN" https://hiveprogram.com/data/_v1/graphqube/wiki.tar.gz
tar -xzvf wiki.tar.gz && rm wiki.tar.gz
mv wiki _data/wiki

# Example 3
mkdir -p ./_results/synthetic/wiki/3
python prep-query.py \
    --query-path ./_data/wiki/query.3.txt \
    --edge-path ./_data/wiki/edgelist.tsv \
    --outdir ./_results/wiki/3

time python main.py \
    --query-path ./_results/wiki/3/query.tsv \
    --edge-path ./_results/wiki/3/edgelist.tsv


# Example 4
mkdir -p ./_results/synthetic/wiki/4
python prep-query.py \
    --query-path ./_data/wiki/query.4.txt \
    --edge-path ./_data/wiki/edgelist.tsv \
    --outdir ./_results/wiki/4

time python main.py \
    --query-path ./_results/wiki/4/query.tsv \
    --edge-path ./_results/wiki/4/edgelist.tsv
