# First arg is number of nodes (e.g.1000000)
# Second arg is output
# Third arg is the graphData directory of mst from HIVE

set -o xtrace
$3/rMatGraph $1 tmp_graph.txt
sort -k1 -n tmp_graph.txt > tmp2_graph.txt
sed -e 's/ /\t/g' tmp2_graph.txt > $2
rm tmp_graph.txt tmp2_graph.txt
