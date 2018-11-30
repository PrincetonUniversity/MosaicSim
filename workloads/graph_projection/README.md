## Purpose
Graph Projection constructs links between unconnected vertices by counting the number of neighbors they have in common.

## Running the Program
The program takes a graph input as an edgelist, as well as other parameters 
such as number of vertices, number of  edges and a boolean value for parameter `simple`
to indicate whether the graph is bi-partite or not.

A sample edgelist, `sample_edgelist.csv`, is provided in the code repo. The
input edgelist is assumed to be a sorted edgelist, sorted based on the sources.

To run with the toy example:
``` srun -N 1 -n 1 -c 8 ./graph_projection --edgelistfile sample_edgelist.csv 
--simple 0 --num-vertices 8 --num-edges 7```

To run the serial version, undefine macro `RUN_WITH_OPENMP`.

### Performance Results:
Since the original D3 dataset is unavailable, the performance result is reported with the following dataset.

The following dataset has been downloaded from:
http://graphchallenge.mit.edu/data-sets

With scale 18 RMAT graph downloaded from 
https://graphchallenge.s3.amazonaws.com/synthetic/graph500-scale18-ef16/graph500-scale18-ef16_adj.tsv.gz

run command:
```srun -N 1 -n 1 -c 1 ./graph_projection  --edgelistfile graph500-scale18-ef16_adj.tsv --num-vertices 262144 --num-edges 4194304```


|# threads  | Execution time | Speedup |
|-----------|:--------------:|:-------:|
|Sequential | 359.29s        |1	       |			
| 2         | 256.346s	     |1.4      |
| 4         | 142.802s       |2.51     |
| 8         | 117.325s       |3.06     |

Platform: Intel(R) Xeon(R) CPU E5-2680 v2 @ 2.80GHz with 20 cores, with 25600 KB cache each core.

#### Expected output
```
Constructing CSR
Construction of CSR done
Started computing graph projection...
Non-bipartite case
Elapsed time: 359.29s
Total newly generated edges: 4865425
```

The generated edges can be printed by enabling macro `PRINT_DEBUG`

### Acknowledgement
I would like to thank Marcin Zalewski for discussion about the latest update.
