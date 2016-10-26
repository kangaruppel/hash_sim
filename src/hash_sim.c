#include "hash_sim.h"
int pass_out_data(data *input,int num_nodes, int rep_factor, int namespace_size)
{	int i;
	for(i=0;i<namespace_size;i++)
		make_data(input+i, i, num_nodes, rep_factor);
		
	return 0; 	
}

int build_node_arr(node *input, int num_nodes, int namespace_size, int num_requests, float write_prob, data *data_arr )
{	int i, j, z;
	message *requests, *cur;
	for(i=0;i<num_nodes;i++) 
	{	//printf("Node %i: \n",i);
		requests=build_request_list(num_requests,namespace_size,data_arr,write_prob);
		make_node(input+i,i,requests);
		//for(cur=(input+i)->Queue;cur;cur=cur->next)
		//	printf("%5d \t %5d \t %5d \t \n", cur->content->ID, cur->content->owner, cur->message_type);		
	}
	
	return 0; 
}

message * build_request_list(int num_requests, int namespace_size, data *data_arr, float write_prob)
{	int i, message_type;
	data *data_addr;
	message *new, *head;
	head=malloc(sizeof(message));
	data_addr=data_arr+(rand()%namespace_size);
	message_type=(rand()%100)>(write_prob*100);
	make_message(head,message_type,data_addr,NULL);
	for(i=1;i<num_requests;i++)
	{	data_addr=data_arr+(rand()%namespace_size);
		message_type=(rand()%100)>(write_prob*100);
		new=malloc(sizeof(message));
		make_message(new,message_type,data_addr,head);
		head=new;
	}
	return head;
}

int all_queues_empty(node *nodes,int num_nodes)
{	int finished=1,i;
	message *cur;
	for(i=0;i<num_nodes;i++)
	{	//printf("Node %i: \n",i);
		//for(cur=(nodes+i)->Queue;cur;cur=cur->next)
			//printf("%5i %5i %5i \n", cur->content->ID, cur->content->owner, cur->message_type);
		if((nodes+i)->Queue)
		{	finished=0;
			break;
		}	
	}
	return finished;
}

float staleness_checker(data *data_arr, long *stale_vector, int namespace_size)
{	int total_var_copies=0, total_stale_copies=0;
	int i,j,stale_copies,stale_time;
	float staleness;
	for(i=0;i<namespace_size;i++)
	{	total_var_copies+=(data_arr+i)->rep_factor;
		stale_copies=0;
		stale_time=0; 
		for(j=0;j<(data_arr+i)->rep_factor;j++)
		{	stale_copies+=!((data_arr+i)->valid_copies[j]);
			stale_time+=(data_arr+i)->invalid_time_total[j];
			
			(data_arr+i)->invalid_time_total[j]=0;
		}
		total_stale_copies+=stale_copies;
		stale_vector[i]+=stale_time;
		//if(stale_vector[i]>1)
			//	printf("out of date!!\n");
	}
	//if(total_stale_copies>1)
		
	staleness=(float)total_stale_copies/(float)total_var_copies;
	//printf("%f\n",staleness);
	//printf("%i %i %f\n",total_stale_copies,total_var_copies, staleness);
	return staleness;
} 

void shuffle(int *array, size_t n)
{
    if (n > 1) {
        size_t i;
	for (i = 0; i < n - 1; i++) {
	  size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
	  int t = array[j];
	  array[j] = array[i];
	  array[i] = t;
	}
    }
	return;
}


long process_requests(int **on_mat,node *nodes, data *data_arr, int num_nodes, int timesteps, int namespace_size)
{	int i, j, k, done=0,num_tried, turnovers=1, on=0, summed=0, num_on=0, on_index;
	long time_to_finish=0;
	int *nodes_on=malloc(sizeof(int)*num_nodes); 
	int *avail_row=malloc(sizeof(int)*num_nodes);
	long *stale_vector=malloc(sizeof(long)*namespace_size);
	float *stale_timelog=malloc(sizeof(float)*timesteps);
	long total_queued_reqs;
	message *cur;
	FILE *output_file;
	if(!stale_vector || !stale_timelog || !avail_row || !nodes_on)
		printf("Error!!!\n\n");
	float total_time, intermed, time_out;
	for(i=0;i<namespace_size;i++)
		stale_vector[i]=0;
	for(i=0;i<timesteps;i++)
		stale_timelog[i]=0;
	for(i=0;i<num_nodes;i++)
		nodes_on[i]=0;
	i=0;
	/*for(i=0;i<timesteps;i++)
	{	for(j=0;j<num_nodes;j++)
		{	printf("%i ",on_mat[i][j]);
			//if(on_mat[i][j]==1)
				//printf("Stop the presses!!\n\n\n");
			
		}
		//printf("\n");
	}*/
	i=0;
	while(!done)
	{	num_on=0; on_index=0; 
		for(k=0;k<num_nodes;k++)
		{	avail_row[k]=on_mat[i][k];
			num_on+=on_mat[i][k];
			if(on_mat[i][k])
			{	nodes_on[on_index]=k;
				on_index++;
				//printf("%i ", k);
			}	
			//if(avail_row[k])
			//	printf("found one!!\n");
		}
		if(on_index>1)
			shuffle(nodes_on,on_index);
		//for(k=0;k<on_index;k++)
		//	printf("%i ",nodes_on[k]);
		num_tried=0;
		//print_queue(nodes+3);
		//printf("\n");
		on=0;
		//printf("\nStart a j:\n");
		while(num_tried<num_on)
		{	j=nodes_on[num_tried]; //pick a random start point, also shouldn't have to worry about cleaning out this array every time...
			total_time=0;
			//num_tried++;
			if(!(nodes+j)->Queue)
			{	num_tried++;
				continue;
			}
			//printf("j=%i\n",j);
			if( on_mat[i][j])
			{	//printf("Checking node %i\n",j);
				on=1;
				num_tried++;
				if(avail_row[j])
				{	
					while(1)
					{	time_out=read_off_queue(j,data_arr,nodes,avail_row,time_to_finish,total_time, num_nodes);
						avail_row[j]=0;
						//printf("Avail_row: ");
						//	for(k=0;k<num_nodes;k++)
						//	printf("%i ",avail_row[k]);
						//printf("\n");
						total_time+=time_out;
						if(total_time>=1 || !(nodes+j)->Queue)
							break;
					}
				}
			}
		}
		//if(num_on>0)
		//	printf("Tried all nodes\n");
	/*	if(on)
		{	summed=0;
			for(k=0;k<num_nodes;k++)
			{	printf("%i ",on_mat[i][k]);
				summed+=on_mat[i][k];
			}
			printf("\n");
			if(summed>1){
				for(k=0;k<num_nodes;k++)
				{	printf("Node=%i\n",k);
					print_queue(nodes+k);
					printf("\n");
				}
			}
			on=0;
		}*/
		done=all_queues_empty(nodes,num_nodes);
		intermed=staleness_checker(data_arr,stale_vector,namespace_size); 
		stale_timelog[time_to_finish]=intermed;
		time_to_finish++;
		i++;
		if(i>=timesteps)
		{	i=0;
			turnovers++;
			stale_timelog=realloc(stale_timelog,sizeof(int)*timesteps*turnovers);
			total_queued_reqs=0;
			for(k=0;k<num_nodes;k++)
			{	cur=(nodes+k)->Queue;
				while(cur)
				{	cur=cur->next;
					total_queued_reqs++;	
				}
			}
			printf("Not dead yet Time=%d Queded requests=%d\n",time_to_finish, total_queued_reqs);
		}
	}
	//output_file=fopen("output.txt","w");
	printf("Stale time log: \n");
	//for(i=0;i<time_to_finish;i++)
	//	printf("%f\n", stale_timelog[i]);
	//printf("Cumulative staleness per variable\n");
	//for(i=0;i<namespace_size;i++)
	//	printf("%ld\n",stale_vector[i]);
	printf("\n");
	//fclose(output_file);
	free(avail_row);
	free(stale_vector);
	free(stale_timelog);
	free(nodes_on);
	return time_to_finish;
		
}


float read_off_queue(int j, data *data_arr, node *nodes, int *on_row, double  global_time, float total_time, int num_nodes)
{	//printf("J=%i\n",j);
	message *packet=(nodes+j)->Queue; //Later right scheduler function to pack in with message type!!!
	if(!packet)
		return 1; 
	message *new_packet, *test_packet;
	data *dummy;
	int ID=packet->content->ID, rep_factor=packet->content->rep_factor, writer=0;
	int *copyholders=malloc(sizeof(int)*packet->content->rep_factor);
	int owner=packet->content->owner;
	int *avail_row=malloc(sizeof(int)*num_nodes);
	int i,k, flag=1, sum_valid=0, extra, copy_index, writer_index;
	float time=0; 
	write *cur_write;
	for(i=0;i<packet->content->rep_factor;i++)
		copyholders[i]=packet->content->copyholders[i];
	for(i=0;i<num_nodes;i++)
		avail_row[i]=on_row[i];
	switch(packet->message_type)
	{	case 0: //Schedule a write
			//Check if it's a writer already.
			/*writer_index=is_writer(packet->content, j);
			if(writer_index>-1)
				writer=1;
			//Check if we can make it a writer, and do so if allowed
			else if(packet->content->num_writers<WRITER_LEVEL)
			{	packet->content->num_writers++; 	
				packet->content->writers=realloc(packet->content->writers,packet->content->num_writers*sizeof(int));
				packet->content->writers[packet->content->num_writers-1]=j; 
				writer=1;
			}*/
			if(MULTI_WRITER==ALL)
				writer=1;
			else if(MULTI_WRITER==COPIES)
				for(i=0;i<rep_factor;i++)
				{	if(copyholders[i]==j)
					{	writer=1;
						break;
					}
				}
			if(j==owner)
				writer=1;
			//packet->content->num_writers++; //increment number of writers of this data (ever), used for versions
			//If the node is allowed to write it... FYI, it'll also get responsibility for updating
			if(writer)
			{	time=.01;
				//Do set up for a new write
				new_write_monitoring(j, nodes,packet->content, global_time);
				//Figure out if this node is a copy holder
				copy_index=is_copyholder(packet->content,j);
				//check if there's time to send the update
				flag=1;
				if(rep_factor>1)
				{	if(total_time>0)
					{	flag=0; //Don't let updates be sent, immediately throw a "4" packet on the queue. 
					}
					else
					{	(nodes+j)->Updates+=1;
						(nodes+j)->Total+=1;
						time=1;
						flag=1;
					}
					time=1; //need to change later if we aren't forcing this to take priority!!
				}
				//sum_valid=1;
				if(flag)
				{	cur_write=(nodes+j)->Active_writes;
					for(i=0;i<rep_factor;i++)
					{	if(on_row[copyholders[i]] && !cur_write->copies_acked[i]) //one of the copies is on and it hasn't acked
						{	cur_write->copies_acked[i]=1;
							(data_arr+ID)->valid_copies[i]=cur_write->version;
							(data_arr+ID)->invalid_time_total[i]=0; //since it was on when someone first issued a write
							(data_arr+ID)->invalid_time_start[i]=0; //reset
							if(copy_index==i) //skip the ack message since it's local
								continue;
							new_packet=malloc(sizeof(message));
							make_message(new_packet,2,packet->content,NULL); //notify of update
							add_query(nodes+copyholders[i],new_packet);
							//avail_row[copyholders[i-1]]=0;
							on_row[copyholders[i]]=0; //<-- replaced line above as a test.... 10-16-16
							
						}
						
					}
				}
				sum_valid=0;
				for(i=0;i<rep_factor;i++)
				{	if(!cur_write->copies_acked[i])
						break;
					sum_valid++;
				}
				if(sum_valid<rep_factor) //Add packet back on
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,4,packet->content,NULL);
					remove_query(nodes+j,packet);
					add_query(nodes+j,new_packet);
				}
				else //pull active write off of the list
					remove_write(nodes+j,cur_write);								
			}
			else //tell owner
			{	if(total_time>0) //don't send anything if not enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				}
				
				time=1;
				/*	for(k=0;k<packet->content->num_writers;k++)
				{	if(on_row[packet->content->writers[k]])
					{	writer=1; //all right, i'm reusing this boolean again, so sue me
						owner=packet->content->writers[k];
						break;	
					}
				}*/
				if(on_row[owner]) //owner is on
				{	new_write_monitoring(owner, nodes,packet->content, global_time);
					new_packet=malloc(sizeof(message));
					make_message(new_packet,0,packet->content,NULL); //need to throw 0 packet on there!!
					add_query(nodes+owner,new_packet); //I shouldn't be leaving "owner" here b/c it's not strictly correct...
					cur_write=(nodes+owner)->Active_writes;
					cur_write->copies_acked[0]=1;
					(data_arr+ID)->valid_copies[0]=cur_write->version;
					(data_arr+ID)->invalid_time_total[0]=0; //since it was on when someone first issued a write
					(data_arr+ID)->invalid_time_start[0]=0; //reset
					(nodes+j)->Hits+=1;
					(nodes+j)->Total+=1;
					(nodes+owner)->Acks++;
					(nodes+owner)->Total++; 
					//avail_row[owner]=0; 
					on_row[owner]=0;
					remove_query(nodes+j,packet);
				}
				else
				{	(nodes+j)->Misses++;
					(nodes+j)->Total++;
				}
				
			}
			break;
		case 1: //Interpret a read 
			flag=0;
			for(i=0;i<rep_factor;i++)
			{	if(copyholders[i]==j)
				{	flag=1; //local copy
					break;
				}
			}
			//if(owner==j) //can probably get rid of this...
			//	flag=1;
			if(flag) //Local read
			{	time=.01;
				remove_query(nodes+j,packet);
				(nodes+j)->Hits++;
				(nodes+j)->Total++;
			}
			else //hunt around for remote read...
			{	if(total_time>0) //Quit if there isn't enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				}
				time=1;
				flag=0;
				for(i=0;i<rep_factor;i++) //check for any copyholders that are on. 
				{	if(on_row[copyholders[i]]) //TODO: randomize this too
					{	new_packet=malloc(sizeof(message));
						make_message(new_packet,3,packet->content,NULL); //notify of update
						add_query(nodes+copyholders[i],new_packet);
						//avail_row[copyholders[i-1]]=0;
						on_row[copyholders[i]]=0;
						remove_query(nodes+j,packet);
						(nodes+j)->Hits++;
						(nodes+j)->Total++;
						flag=1;
						break;
					}
				}
				if(!flag)
				{	(nodes+j)->Misses++;
					(nodes+j)->Total++;		
				}
			}
			//do valid check here!!!!!!!!!!!!!!
			//if(owner==j)//this node is the data owner
			//	i=0;
			//else
			//for(i=0;i<rep_factor, copyholders[i]!=j;i++);	//run through copies and find the index of this node
			
			//since we broke both of the prio for loop and haven't touched i since... 
			if(flag)
			{	if((data_arr+ID)->valid_copies[i]!=(data_arr+ID)->num_writers)	
					(data_arr+ID)->invalid_accesses++;
			}
			break;
		case 2: //scheduling consistency update
			if(total_time>0)//--> REALLY shouldn't happen... 
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			remove_query(nodes+j,packet);
			(nodes+j)->Updates++;
			(nodes+j)->Total++;
			time=1;
			break;
		case 3: //read ack
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			remove_query(nodes+j,packet);
			(nodes+j)->Acks++;
			(nodes+j)->Total++;
			time=1;
			break;
		case 4: 
			if(total_time>0)
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			//find correct update in list of active writes on node
			for(cur_write=(nodes+j)->Active_writes;cur_write->next, cur_write->ID!=packet->content->ID;cur_write=cur_write->next);
			(nodes+j)->Updates++;
			(nodes+j)->Total++;
			time=1;
			
			for(i=0;i<rep_factor;i++)
					{	if(on_row[copyholders[i]] && !cur_write->copies_acked[i]) //one of the copies is on and it hasn't acked
						{	cur_write->copies_acked[i]=1;
							(data_arr+ID)->valid_copies[i]=cur_write->version;
							(data_arr+ID)->invalid_time_total[i]=0; //since it was on when someone first issued a write
							(data_arr+ID)->invalid_time_start[i]=0; //reset
							if(copy_index==i) //skip the ack message since it's local
								continue;
							new_packet=malloc(sizeof(message));
							make_message(new_packet,2,packet->content,NULL); //notify of update
							add_query(nodes+copyholders[i],new_packet);
							//avail_row[copyholders[i-1]]=0;
							on_row[copyholders[i]]=0; //<-- replaced line above as a test.... 10-16-16
							
						}
						
					}
			sum_valid=0;
				for(i=0;i<rep_factor;i++)
				{	if(!cur_write->copies_acked[i])
						break;
					sum_valid++;
				}
				if(sum_valid<rep_factor) //Add packet back on
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,4,packet->content,NULL);
					remove_query(nodes+j,packet);
					add_query(nodes+j,new_packet);
				}
				else //pull active write off of the list
					remove_write(nodes+j,cur_write);
			break;
		default: 
			printf("Something screwed up really badly!!!\n");
			exit(0);
	}
	free(copyholders);
	free(avail_row);
	return time;
	
}

int new_write_monitoring(int j, node *nodes, data *content, double global_time)
{	write *new=malloc(sizeof(write));
	(nodes+j)->Hits+=1;
	(nodes+j)->Total+=1;
	content->num_writers++;
	if(!new)
		return -1;
	make_write(new,content->ID, content->num_writers, content->rep_factor);
	add_write(nodes+j, new);
	content->mod_times=realloc(content->mod_times,sizeof(int)*content->num_writers);
	content->mod_times[content->num_writers-1]=global_time;
	return 0; 
}



