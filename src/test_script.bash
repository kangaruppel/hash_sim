#loop through and check the different queue lengths
NUMS="10 50 100 500 1000 5000 10000"
for i in $NUMS
do 
	echo "Queue length= $i \n" 
	#./hash_sim ../on_mat_new.csv -q $i #>> single_writer.out
	#./hash_sim ../on_mat_new.csv -q $i -m 2  #>> copy_writer.out
done

#comments comments 
#for v in 10 50 100 500 1000 5000 10000
#do 
#	echo "Namespace Size= $i \n" 
#	hash_sim ../on_mat_new.csv -v $i >> single_writer.out
#	hash_sim ../on_mat_new.csv -v $i -m 2  >>  copy_writer.out
#done
#
#end of file 
