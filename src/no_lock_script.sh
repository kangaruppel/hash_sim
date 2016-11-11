#!/bin/bash
NUMS="10 25 50 100 500"
for i in $NUMS
do
	echo "$i"
	./hash_sim_2 ../on_mat_new.csv -v $i -fs stale_out_v$i_m0.txt -fa stat_out_v$i_m0.txt -s 1008
	./hash_sim_2 ../on_mat_new.csv -v $i -m 2 -fs stale_out_v$i_m2.txt -fa stat_out_v$i_m2.txt -s 1008
done