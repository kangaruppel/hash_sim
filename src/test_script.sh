#!/bin/bash

for i in 10 50 100 500 1000 5000 10000
do 
	hash_sim ../../on_mat_new.csv -q $i >> single_writer.out
	hash_sim ../../on_mat_new.csv -q $i -m 2  >> single_writer.out
end

for v in 10 50 100 500 1000 5000 10000do 
	hash_sim ../../on_mat_new.csv -v $i >> single_writer.out
	hash_sim ../../on_mat_new.csv -v $i -m 2  >> single_writer.out
end