#include "hash_sim.h"



int main(int argc, char *argv[])
{	FILE *on_mat_file;
	int  num_nodes, num_requests=1000, namespace_size=1000, rep_factor, time_outsteps=1;
	int hits, misses, updates, acks, total, invalid_accesses;
	int **on_mat;
	float write_probability=.25, peak_staleness=0;
	char test,junk;
	int i=0, j=0, test2;
	long time_out=0;
	time_t rawtime; 
	struct tm * timeinfo; 
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	RETRY_LEVEL=0;
	MULTI_WRITER=0;
	FINISH_ALL_UPDATES=1;
	NEWER_WRITE_NOT_REQ=0;
	STALE_OUTPUT_FILE=fopen("stale_output.txt","a");
	STAT_OUTPUT_FILE=fopen("stat_output.txt","a");
	RAND_SEED=time(NULL);
	LOCKS_ON=0; 
	if(argc < 2)
	{	fprintf(stderr, "Not enough input arguments!", -1);
		return -1; 
	}
	for(i=0;i<argc;i++)
	{	if(argv[i][0]=='-')
		{	switch(argv[i][1])
			{	case 'r': //retry level
					RETRY_LEVEL=atoi(argv[i+1]);
					break;
				case 'm': //multi writer
					MULTI_WRITER=atoi(argv[i+1]);
					break;	
				case 'q': //queue size initial
					num_requests=atoi(argv[i+1]);
					break;
				case 'v': //variable number
					namespace_size=atoi(argv[i+1]);
					break; 
				case 'k': //replication factor
					rep_factor=atoi(argv[i+1]);
					break; 
				case 'p': //probability of write
					write_probability=atoi(argv[i+1]);
					break; 
				case 'u': //finish all updates or leave if all other requests filled
					FINISH_ALL_UPDATES=atoi(argv[i+1]);
					break;
				case 'n': //newer write requirement
					NEWER_WRITE_NOT_REQ=atoi(argv[i+1]);
					break; 
				case 'f': //writing to a different file
					if(argv[i][2] == 's')
					{	fclose(STALE_OUTPUT_FILE);
						STALE_OUTPUT_FILE=fopen(argv[i+1],"a");
					}
					else
					{	fclose(STAT_OUTPUT_FILE);
						STAT_OUTPUT_FILE=fopen(argv[i+1],"a");
					}
					break;
				case 's': //seed for random number generator
					RAND_SEED=atoi(argv[i+1]);
					break;
				case 'l': //turning locks on 
					LOCKS_ON=atoi(argv[i+1]);
					break; 
			}
			
		}
		
	}
	fprintf(STALE_OUTPUT_FILE,"-----------Run at %s--------------\n",asctime(timeinfo));
	//srand(3658);
	srand(RAND_SEED);
	//srand(time(NULL));
	//printf("Enter number of requests, namespace size, replication factor, and write probability.\n");
	//scanf("%i %i %i %f", &num_requests,&namespace_size,&rep_factor, &write_probability);
	//printf("%i %i %i %f\n", num_requests, namespace_size, rep_factor, write_probability);
	on_mat_file=fopen(argv[1],"r");
	i=0;
	//Get num_nodes --> width of input array
	while(1)
	{	fscanf(on_mat_file,"%c", &test);
		printf("%c",test);
		i++;
		if(test=='\n')
		{	num_nodes=i/2;
			break;
		}
	}
	printf("num_nodes=%i\n",num_nodes);
	if(!rep_factor)
		rep_factor=num_nodes/5;
	//Get number of time_outsteps -->length on input array
	while((test=fgetc(on_mat_file))!=EOF)
	{	 //printf("%c",test);
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
	time_out=process_requests(on_mat,node_arr,data_arr,num_nodes,time_outsteps,namespace_size, &peak_staleness);
	
	hits=0;misses=0;updates=0;acks=0;total=0;
	for(i=0;i<num_nodes;i++)
	{	hits+=(node_arr+i)->Hits;
		misses+=(node_arr+i)->Misses;
		updates+=(node_arr+i)->Updates;
		acks+=(node_arr+i)->Acks;
		total+=(node_arr+i)->Total;
	}
	invalid_accesses=0;
	fprintf(STAT_OUTPUT_FILE,"Copy # | Invalidated Cnt | Avg Time Invalid | Avg Time Invalid & On \n");
	for(i=0;i<namespace_size;i++)
	{	invalid_accesses+=(data_arr+i)->invalid_accesses;
		printf("----------Variable %i, Written %i Times, Invalid Accesses %i----------\n",i, (data_arr+i)->num_writers, (data_arr+i)->invalid_accesses);
		for(j=0;j<(data_arr+i)->rep_factor;j++)
		{	printf("copy %i: Inval %i Times, Avg Time Inval: %.2f Avg Time Inval & On: %.2f \n",j,(data_arr+i)->invalidated_cnt[j],(data_arr+i)->avg_time_invalid[j],(data_arr+i)->avg_time_on_while_invalid[j]); 
			fprintf(STAT_OUTPUT_FILE,"%7i %16i %17.2f %22.2f\n",j,(data_arr+i)->invalidated_cnt[j],(data_arr+i)->avg_time_invalid[j],(data_arr+i)->avg_time_on_while_invalid[j]); 
		}
	}
	printf("time_out to finish= %d\n",time_out);
	printf("Hits=%i\nMisses=%i\nUpdates=%i\nAcks=%i\nTotal=%i\n",hits,misses,updates,acks,total);
	printf("Reads of invalid data: %i\n",invalid_accesses);
	fprintf(STAT_OUTPUT_FILE,"Time to finish | Hits | Misses | Invalid Access | Peak Staleness\n %14d %5d %7d %8d %8.2f",time_out, hits, misses, invalid_accesses, peak_staleness);
	
//	fclose(STALE_OUTPUT_FILE);
	fclose(STAT_OUTPUT_FILE);
	fclose(on_mat_file);
	/*rint_queue(node_arr+3);
	printf("\n");
	remove_query(node_arr+3,(node_arr+2)->Queue->next->next);
	print_queue(node_arr+3);*/
	for(i=0;i<time_outsteps;i++)
		free(on_mat[i]);
	free(on_mat);
	for(i=0;i<num_nodes;i++)
	{
		free_node(node_arr+i);
	}
	free(node_arr);
	for(i=0;i<namespace_size;i++)
	{
		free_data(data_arr+i);
	}
	free(data_arr);
	
	return 0; 
}

