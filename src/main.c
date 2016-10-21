#include "hash_sim.h"


//So far, we need a bazillion inputs to this... TODO: streamline inputs!
int main(int argc, char *argv[])
{	FILE *on_mat_file;
	int  num_nodes, num_requests, namespace_size, rep_factor, time_outsteps=1;
	int hits, misses, updates, acks, total, invalid_accesses;
	int **on_mat;
	float write_probability;
	char test,junk;
	int i=0, j=0, test2;
	long time_out=0; 
	if(argc < 2)
	{	fprintf(stderr, "Not enough input arguments!", -1);
		return -1; 
	}
	//srand(3658);
	srand(time(NULL));
	printf("Enter number of requests, namespace size, replication factor, and write probability.\n");
	scanf("%i %i %i %f", &num_requests,&namespace_size,&rep_factor, &write_probability);
	printf("%i %i %i %f\n", num_requests, namespace_size, rep_factor, write_probability);
	on_mat_file=fopen(argv[1],"r");
	//Get num_nodes --> width of input array
	while(1)
	{	fscanf(on_mat_file,"%c", &test);
		i++;
		if(test=='\n')
		{	num_nodes=i/2;
			break;
		}
	}
	printf("num_nodes=%i\n",num_nodes);
	//Get number of time_outsteps -->length on input array
	while((test=fgetc(on_mat_file))!=EOF)
	{	 
		if(test=='\n')
			time_outsteps++;
	}
	printf("num time_outsteps=%i\n", time_outsteps);
	fclose(on_mat_file);
	on_mat_file=fopen(argv[1],"r");
	on_mat=(int **)malloc(sizeof(int *)*time_outsteps);
	//read in data for on_mat
	for(i=0;i<time_outsteps;i++)
		on_mat[i]=(int *)malloc(sizeof(int)*num_nodes);
		
	for(i=0;i<time_outsteps;i++)
	{	for(j=0; j<num_nodes;j++)
		{	fscanf(on_mat_file,"%d%c",&(on_mat[i][j]),&junk);
			if(junk=='\r')
				junk=fgetc(on_mat_file); //grab new line after carriage return b/c csv files suck...
			//if(on_mat[i][j]==1)
			//	printf("Stop the presses!\n");
			//printf("%i ",on_mat[i][j]);
		}
		//printf("\n");
	} 
	
	//build the data array
	data *data_arr=malloc(sizeof(data)*namespace_size);
	pass_out_data(data_arr,num_nodes,rep_factor,namespace_size);
	node *node_arr=malloc(sizeof(node)*num_nodes);
	build_node_arr(node_arr, num_nodes, namespace_size,num_requests, write_probability, data_arr);
	time_out=process_requests(on_mat,node_arr,data_arr,num_nodes,time_outsteps,namespace_size);
	printf("time_out to finish= %d\n",time_out);
	hits=0;misses=0;updates=0;acks=0;total=0;
	for(i=0;i<num_nodes;i++)
	{	hits+=(node_arr+i)->Hits;
		misses+=(node_arr+i)->Misses;
		updates+=(node_arr+i)->Updates;
		acks+=(node_arr+i)->Acks;
		total+=(node_arr+i)->Total;
	}
	invalid_accesses=0;
	for(i=0;i<namespace_size;i++)
		invalid_accesses+=(data_arr+i)->invalid_accesses;
	printf("Hits=%i\nMisses=%i\nUpdates=%i\nAcks=%i\nTotal=%i\n",hits,misses,updates,acks,total);
	printf("Reads of invalid data: %i\n",invalid_accesses);
	/*rint_queue(node_arr+3);
	printf("\n");
	remove_query(node_arr+3,(node_arr+2)->Queue->next->next);
	print_queue(node_arr+3);*/
	for(i=0;i<time_outsteps;i++)
		free(on_mat[i]);
	free(on_mat);
	for(i=0;i<namespace_size;i++)
	{
		free_data(data_arr+i);
	}
	free(data_arr);
	for(i=0;i<num_nodes;i++)
	{
		free_node(node_arr+i);
	}
	free(node_arr);
	fclose(on_mat_file);
	return 0; 
}

