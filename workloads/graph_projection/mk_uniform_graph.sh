# First arg is number of edges (e.g. 5000000)
# Second arg is number of nodes (e.g.1000000)
# Third arg is output
# Fourth arg is the graphData directory of mst from HIVE

set -o xtrace
$4/randLocalGraph -d 3 -m $1 $2 tmp_graph.txt
$4/addWeights tmp_graph.txt tmp2_graph.txt
tail -n +2 tmp2_graph.txt > tmp_graph.txt
sort -k1 -n tmp_graph.txt > tmp2_graph.txt
sed -e 's/ /\t/g' tmp2_graph.txt > $3
rm tmp_graph.txt tmp2_graph.txt
