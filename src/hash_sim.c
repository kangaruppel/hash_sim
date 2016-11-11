#include "hash_sim.h"
int pass_out_data(data *input,int num_nodes, int rep_factor, int namespace_size)
{	int i;
	for(i=0;i<namespace_size;i++)
		make_data(input+i, i, num_nodes, rep_factor);
		
	return 0; 	
}

int build_node_arr(node *input, int num_nodes, int namespace_size, int num_requests, float write_prob, data *data_arr )
{	int i, j, z,entry,ID;
	message *requests, *cur, *lock_message;
	data *stuff; 
	for(i=0;i<num_nodes;i++) 
	{	//printf("Node %i: \n",i);
		requests=build_request_list(num_requests,namespace_size,data_arr,write_prob,i);
		make_node(input+i,i,requests);
		if(LOCKS_ON)
		{	for(j=0;j<num_requests*write_prob*.25;j++)
			{	entry=rand()%(num_requests+j);
				ID=rand()%namespace_size;
				lock_message=malloc(sizeof(message));
				stuff=malloc(sizeof(data));
				stuff=data_arr+ID;
				make_message(lock_message,i,LOCK_REQ,stuff,NULL);
				add_query_special(input+i,lock_message,NULL, 0,entry);
			}
			//(input+i)->lock_acks=malloc(sizeof(int *)*(data_arr->rep_factor*namespace_size/num_nodes)); //fix this later to handle different rep factors per data
			//(input+i)->release_acks=malloc(sizeof(int *)*(data_arr->rep_factor*namespace_size/num_nodes));
		}//for(cur=(input+i)->Queue;cur;cur=cur->next)
		//	printf("%5d \t %5d \t %5d \t \n", cur->content->ID, cur->content->owner, cur->message_type);		
	}
	
	return 0; 
}

message * build_request_list(int num_requests, int namespace_size, data *data_arr, float write_prob, int sender)
{	int i, message_type,pos;
	data *data_addr;
	message *new, *head;
	int num_locks=num_requests*.25*write_prob; 
	head=malloc(sizeof(message));
	data_addr=data_arr+(rand()%namespace_size);
	message_type=(rand()%100)>(write_prob*100);
	make_message(head,sender,message_type,data_addr,NULL);
	for(i=1;i<num_requests;i++)
	{	data_addr=data_arr+(rand()%namespace_size);
		message_type=(rand()%100)>(write_prob*100);
		new=malloc(sizeof(message));
		make_message(new,sender,message_type,data_addr,head);
		head=new;
	}
	return head;
}

int all_queues_empty(node *nodes,int num_nodes)
{	int finished=1,i;
	message *cur,*temp;
	for(i=0;i<num_nodes;i++)
	{	//printf("Node %i: \n",i);
		//for(cur=(nodes+i)->Queue;cur;cur=cur->next)
			//printf("%5i %5i %5i \n", cur->content->ID, cur->content->owner, cur->message_type);
		if((nodes+i)->Queue)
		{	if(FINISH_ALL_UPDATES)
			{	finished=0;
				break;
			}
			else //If the simulation finishes if only update messages are left
			{	for(temp=(nodes+i)->Queue;temp;temp=temp->next)
				{	if(!temp)
						break;
					if((temp)->message_type!=4)
					{	finished=0;
						break;
					}
				}
				if(finished==0)
					break;
			}
			
		}	
	}
	return finished;
}

float staleness_checker(data *data_arr, int namespace_size, int global_time, int *on_row)
{	int total_var_copies=0, total_stale_copies=0, time_invalid=0;
	int i,j,k,stale_copies,stale_time, power_state,last_power_state,last_valid_state, valid_state=1;
	float staleness;
	for(i=0;i<namespace_size;i++)
	{	total_var_copies+=(data_arr+i)->rep_factor;
		stale_copies=0;
		if((data_arr+i)->num_writes_in_timestep > 1)
		{	for(j=0;j<(data_arr+i)->rep_factor;j++)
				(data_arr+i)->valid_copies[j]=0; //invalidate all the copies...	
		}
		for(j=0;j<(data_arr+i)->rep_factor;j++)//roll through all the copies
		{	 
			valid_state=1; 
			last_valid_state=(data_arr+i)->last_valid_state[j];
			last_power_state=(data_arr+i)->last_onoff_state[j];
			power_state=on_row[(data_arr+i)->copyholders[j]]; //check if copyholder is on or off
			if(!(data_arr+i)->valid_copies[j])//copy_versions[j]!=(data_arr+i)->num_writers) //check if copy is stale
			{	stale_copies++;
				//if(j==0)
				//	printf("but why?\n");
				valid_state=0;
			}
			if(global_time>1076050)
				k++;
			if(!valid_state && last_valid_state)//If newly invalid, inc invalidated_cnt
				(data_arr+i)->invalidated_cnt[j]++;
			if(!valid_state && !last_power_state && power_state)//if invalid and going from off to on TODO: use this to profile on times while invalid 
				(data_arr+i)->on_while_invalid_cnt[j]++; 
			if(!valid_state && power_state)//If on and invalid 
				(data_arr+i)->time_on_and_invalid[j]++;
			if(valid_state && !last_valid_state && (data_arr+i)->invalid_time_start[j])//If newly valid, run through and update avgs
			{	time_invalid=global_time-(data_arr+i)->invalid_time_start[j]; //TODO: make sure invalid_time_start always > 0 
				(data_arr+i)->invalid_time_start[j]=0;
				(data_arr+i)->avg_time_on_while_invalid[j]=((data_arr+i)->avg_time_on_while_invalid[j]*((data_arr+i)->invalidated_cnt[j]-1)+(data_arr+i)->on_while_invalid_cnt[j])/(data_arr+i)->invalidated_cnt[j];
				(data_arr+i)->avg_time_invalid[j]=((data_arr+i)->avg_time_invalid[j]*((data_arr+i)->invalidated_cnt[j]-1)+time_invalid)/(data_arr+i)->invalidated_cnt[j];
			}
			(data_arr+i)->last_onoff_state[j]=power_state;
			(data_arr+i)->last_valid_state[j]=valid_state; 
		}
		total_stale_copies+=stale_copies;
		(data_arr+i)->num_writes_in_timestep=0; //clean out for next time step. 
		//stale_vector[i]+=stale_time;
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


long process_requests(int **on_mat,node *nodes, data *data_arr, int num_nodes, int timesteps, int namespace_size, float *peak_staleness)
{	int i, j, k, done=0,num_tried, turnovers=1, on=0, summed=0, num_on=0, on_index;
	long time_to_finish=0;
	int *nodes_on=malloc(sizeof(int)*num_nodes); 
	int *avail_row=malloc(sizeof(int)*num_nodes);
	int *on_row=malloc(sizeof(int)*num_nodes); 
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
		{	avail_row[k]=on_mat[i][k]; //Build initial row of nodes that are on
			on_row[k]=on_mat[i][k]; //build duplicate for staleness processing later
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
			//total_time=0;
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
				if(on_row[j]) 
				{	total_time=0; 
					while(1)
					{	time_out=read_off_queue(j,data_arr,nodes,avail_row,time_to_finish,total_time, num_nodes);
						//avail_row[j]=0;
						//printf("Avail_row: ");
						//	for(k=0;k<num_nodes;k++)
						//	printf("%i ",avail_row[k]);
						//printf("\n");
						total_time+=time_out;
						if(total_time>=1 || !(nodes+j)->Queue)
						{	avail_row[j]=0; //just added here 11-3 at 10:56	
							break;
						}
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
		if(done)
			k++;
		intermed=staleness_checker(data_arr,namespace_size,time_to_finish, on_row); 
		if(intermed> *peak_staleness)
			*peak_staleness=intermed;
		if(intermed<0)
			printf("Wrong wrong wrong!!\n\n");
		stale_timelog[time_to_finish]=intermed;
		if(!(time_to_finish%50))
		fprintf(STALE_OUTPUT_FILE,"%d : %f\n", time_to_finish,intermed);
		time_to_finish++;
		i++;
		if(i>=timesteps)
		{	i=0;
			turnovers++;
			stale_timelog=realloc(stale_timelog,sizeof(int)*timesteps*turnovers);
			total_queued_reqs=0;
			if(!(turnovers%5))
			{	for(k=0;k<num_nodes;k++)
				{	cur=(nodes+k)->Queue;
					while(cur)
					{	cur=cur->next;
						total_queued_reqs++;	
					}
				}
				printf("Not dead yet Time=%d Queded requests=%d\n",time_to_finish, total_queued_reqs);
			}
		}
	}
	fprintf(STALE_OUTPUT_FILE,"--------------------------------------------------\n\n");
	fclose(STALE_OUTPUT_FILE);
	//output_file=fopen("output.txt","w");
	//printf("Stale time log: \n");
	//for(i=0;i<time_to_finish;i++)
	//	printf("%f\n", stale_timelog[i]);
	//printf("Cumulative staleness per variable\n");
	//for(i=0;i<namespace_size;i++)
	////	printf("%ld\n",stale_vector[i]);
	printf("\n");
	//fclose(output_file);
	free(on_row);
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
	int i,k, flag=1, sum_valid=0, extra, copy_index=-1, writer_index, copy_index2=-1;
	float time=.01; 
	int sender=packet->sender; 
	int msg_type=packet->message_type;
	write *cur_write;
	lock *cur_lock=packet->content->data_lock;
	int start_lock_holder=cur_lock->holder;
	for(i=0;i<packet->content->rep_factor;i++)
		copyholders[i]=packet->content->copyholders[i];
	for(i=0;i<num_nodes;i++)
		avail_row[i]=on_row[i];
	//Only applies if j is a stakeholder for this lock... otherwise the other nodes
	//have to deal with the fact that some node is trying to get at locked data. 
	i=is_stakeholder(cur_lock,j);
	
	//if(j==21 && packet->message_type==GRANTED)
	//	printf("Here we go!\n");
	if(cur_lock->ID==43 && j==18 && global_time>13450000)
		k++; 
	//if(cur_lock->ID==43 && cur_lock->holder==-1 && cur_lock->stakeholders_locked[3] && cur_lock->stakeholders_release_acked[3])
	//	printf("Wait, what? Why? How? \n"); 
	if(i>-1 && cur_lock->holder!=packet->sender && (packet->message_type==WRITE || packet->message_type==READ || packet->message_type==SEND_UPDATES) && cur_lock->stakeholders_locked[i]) //for now we're not doing any dynamic reordering, so this just blocks...
	{	time=1; //TODO: change if we're doing something other than blocking with these
		if(cur_lock->holder==-1 && cur_lock->stakeholders_locked[0])
			printf("Why??????????\n");
		free(avail_row);
		free(copyholders);
		return time;
	}
	switch(packet->message_type)
	{	case WRITE: //Schedule a write
			if(MULTI_WRITER==ALL)
				writer=1;
			else if(MULTI_WRITER==COPIES)
			{	//Figure out if this node is a copy holder
				copy_index=is_copyholder(packet->content,j);
				if(copy_index>=0)
					writer=1;
			}
			if(j==owner)
				writer=1;
			if(writer)
			{	time=.01;
				//Do set up for a new write, includes incrementing hits
				new_write_monitoring(j, packet->sender, nodes,packet->content, global_time);
				if((nodes+j)->Active_writes==0)
					printf("Look, ya screwed up here too!\n");
				//check if there's time to send the update
				flag=0;
				cur_write=(nodes+j)->Active_writes;
				if(rep_factor>1)
				{	if(total_time>0)
					{	i=is_copyholder(packet->content,j);
						if(i>-1) //update if it's a local copy so it's not invalid anymore
							(data_arr+ID)->copy_versions[i]=cur_write->version;
						flag=0; //Don't let updates be sent, immediately throw a "4" packet on the queue. 
					}
					else
					{	(nodes+j)->Updates+=1; //since there's time to send a broadcast update
						(nodes+j)->Total+=1;
						time=1;
						flag=1;
					}
					time=1; //need to change later if we aren't forcing this to take priority!!
				}
				sum_valid=0;
				if(flag) //if sending off node 
				{	//check in with ALL the copyholders, but ignore the ones who already ack'd
					for(i=0;i<rep_factor;i++) //TODO: package into a function!!!
					{	if((on_row[copyholders[i]] || copyholders[i]==j || copyholders[i]==packet->sender) && !cur_write->copies_acked[i]) //one of the copies is on and it hasn't acked
						{	cur_write->copies_acked[i]=1;
							on_row[copyholders[i]]=0;
							if(copyholders[i]==j) //skip the ack message since it's local
							{	(data_arr+ID)->copy_versions[i]=cur_write->version;
								(data_arr+ID)->valid_copies[i]= (packet->sender==(data_arr+ID)->most_recent_writer);
								continue;
							}
							new_packet=malloc(sizeof(message));
							make_message(new_packet,packet->sender, UPDATE,packet->content,NULL); //notify of update
							add_query(nodes+copyholders[i],new_packet);				
						}
						
					}
				}
				//Count how many copies ack'd over time.
				for(i=0;i<rep_factor;i++)
				{	if(!cur_write->copies_acked[i])
						break;
					sum_valid++;
				}
				//sum_valid=0;
				if(sum_valid<rep_factor) //Add send update packet
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,packet->sender,SEND_UPDATES,packet->content,NULL); 
					add_query(nodes+j,new_packet);
					if((nodes+j)->Active_writes==0)
						printf("Womp... Error\n");
					remove_query(nodes+j,packet);
				}
				else //pull active write off of the list
				{	remove_write(nodes+j,cur_write);	
					remove_query(nodes+j,packet);
				}
				if(!(nodes+j)->Active_writes && (nodes+j)->Queue->message_type==4)
					printf("null active write and update ordered... Game over\n");
			}
			else //tell owner or copyholders
			{	if(total_time>0) //don't send anything if not enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				}
				flag=0; 
				time=1;
				for(i=0;i<rep_factor;i++)
				{	if(!MULTI_WRITER && i>0)
						break;
					if(on_row[copyholders[i]]==1 && !cur_lock->stakeholders_locked[i]) //some copyholder is on, TODO: randomize this!!
					{	new_packet=malloc(sizeof(message));
						make_message(new_packet,j,WRITE,packet->content,NULL); //need to throw 0 packet on there!!
						add_query(nodes+copyholders[i],new_packet); //I shouldn't be leaving "owner" here b/c it's not strictly correct...
						(nodes+j)->Hits+=1;
						(nodes+j)->Total+=1;
						(nodes+copyholders[i])->Acks++;
						(nodes+copyholders[i])->Total++; 
						on_row[copyholders[i]]=0;
						remove_query(nodes+j,packet);
						flag=1;
						break;
					}
				}
				if(!flag)//meaning we didn't get a hold of any copies 
				{	//remove_query(nodes+j,packet);
					(nodes+j)->Misses++;
					(nodes+j)->Total++;
				}	
			}
			break;
		case READ: //Interpret a read 
			flag=0;
			for(i=0;i<rep_factor;i++)
			{	if(copyholders[i]==j)
				{	flag=1; //local copy
					break;
				}
			}
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
				for(i=0;i<rep_factor;i++) //check for any copyholders that are on with released locks on the data. 
				{	if(on_row[copyholders[i]] && (!cur_lock->stakeholders_locked[i] || j==cur_lock->holder)) //TODO: randomize this too
					{	new_packet=malloc(sizeof(message));
						make_message(new_packet,j,READ_ACK,packet->content,NULL); //notify of update
						add_query(nodes+copyholders[i],new_packet);
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
			//since we broke both of the prior for loops and haven't touched i since... 
			if(flag)
			{	for(k=0;k<rep_factor;k++)
				{	if((data_arr+ID)->copy_versions[k]!=(data_arr+ID)->num_writers)
					{	(data_arr+ID)->risk_window_hits++; 
						break; 
					}
				}
				(data_arr+ID)->total_reads++; 
				if((data_arr+ID)->copy_versions[i]!=(data_arr+ID)->num_writers)	
					(data_arr+ID)->invalid_accesses++;
			}
			break;
		case UPDATE: //scheduling consistency update
			if(total_time>0)//--> REALLY shouldn't happen... 
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			copy_index=is_copyholder(packet->content,j);
			copy_index2=is_copyholder(packet->content,packet->sender);
			if(copy_index>-1)//if j is a copyholder and it's getting updated
			{	if((data_arr+ID)->copy_versions[copy_index]<(data_arr+ID)->copy_versions[copy_index2] || NEWER_WRITE_NOT_REQ)
				{	(data_arr+ID)->copy_versions[copy_index]=(data_arr+ID)->copy_versions[copy_index2];
				}
				(data_arr+ID)->valid_copies[copy_index]= (packet->sender==(data_arr+ID)->most_recent_writer);
			}
			remove_query(nodes+j,packet);
			(nodes+j)->Updates++;
			(nodes+j)->Total++;
			time=1;
			break;
		case READ_ACK: //read ack
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
		case SEND_UPDATES: 
			if(total_time>0)
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			if(!(nodes+j)->Active_writes)
				printf("Ya done screwed it up\n");
			//find correct update in list of active writes on node
			for(cur_write=(nodes+j)->Active_writes;cur_write->next;cur_write=cur_write->next)
			{	 if(cur_write->ID==packet->content->ID)
					 break; 
			}
			if(cur_write->ID!=packet->content->ID)
				printf("Bad! no matching update found\n");
			(nodes+j)->Updates++;
			(nodes+j)->Total++;
			time=1;
			copy_index=is_copyholder(packet->content,j);
			for(i=0;i<rep_factor;i++)
			{	if(on_row[copyholders[i]] && !cur_write->copies_acked[i]) //one of the copies is on and it hasn't acked
				{	cur_write->copies_acked[i]=1;
					if((data_arr+ID)->copy_versions[i]<cur_write->version) //To ensure that a newer update isn't overwritten. 
						(data_arr+ID)->copy_versions[i]=cur_write->version;
					if(copyholders[i]==j) //skip the ack message since it's local
						continue;	//realistically there's no way the local shouldn't already be ack'd. 
					new_packet=malloc(sizeof(message));
					make_message(new_packet,packet->sender,UPDATE,packet->content,NULL); //notify of update
					add_query(nodes+copyholders[i],new_packet);
					on_row[copyholders[i]]=0;  
				}	
			}
			sum_valid=0;
			for(i=0;i<rep_factor;i++)
			{	if(!cur_write->copies_acked[i])
					break;
				sum_valid++;
			}
			if(sum_valid>=rep_factor) //pull active write off of the list
			{	remove_write(nodes+j,cur_write);
				remove_query(nodes+j,packet);
			}
			break;
		case LOCK_REQ: //lock request
			owner=cur_lock->owner;
			//print_queue(nodes+j,5);
			if(owner!=j)//Try to get in touch with lock owner...
			{	if(total_time>0) //don't send anything if not enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				} 
				
				if(on_row[owner] /*&& (!(nodes+owner)->Queue || (nodes+owner)->Queue->message_type< 5 || (nodes+owner)->Queue->content->ID==cur_lock->ID)*/) //just added... sooo kludgy!!
				{	
					new_packet=malloc(sizeof(message));
					make_message(new_packet,j,LOCK_REQ,packet->content,NULL); //need to throw 5 packet on there!!
					add_query(nodes+owner,new_packet); //I shouldn't be leaving "owner" here b/c it's not strictly correct...
					(nodes+j)->Hits+=1;
					(nodes+j)->Total+=1;
					(nodes+owner)->Acks++;
					(nodes+owner)->Total++;  
					on_row[owner]=0;
					remove_query(nodes+j,packet);
				}
				else
				{	(nodes+j)->Misses++;
					(nodes+j)->Total++;
				}
			}
			else //this is the owner
			{	if(cur_lock->stakeholders_locked[0] && cur_lock->holder!=packet->sender)//if someone has touched the lock
					flag=DENIED;//DENIED signal- lock already held by another node
				else if( packet->next && packet->next->message_type>5  && cur_lock->ID!=packet->next->content->ID)
					flag=DENIED;//send denied... this node is busy
				else //if data hasn't been touched or it's the lock requestor
				{	if(!cur_lock->stakeholders_locked[0])//totally new lock request needs to be set up
					{	cur_lock->version++;
						cur_lock->holder=packet->sender;
						//=realloc((nodes+j)->lock_acks,sizeof(int)*cur_lock->num_stakeholders); 
						//remove_lock_release_notice(nodes+j,cur_lock->ID); //to get rid of queued up release commands
						for(i=0;i<cur_lock->num_stakeholders;i++)
							cur_lock->stakeholders_set_acked[i]=0; 
					}
					else
						remove_lock_set(nodes+j,cur_lock->ID); //to get rid of old set packets hanging around
					for(i=0;i<cur_lock->num_stakeholders;i++) //check if any of the stakeholders is on
					{	if((on_row[cur_lock->stakeholders[i]] || cur_lock->stakeholders[i]==j)&& !cur_lock->stakeholders_set_acked[i]) //one of the stakeholders is on and it hasn't acked
						{	cur_lock->stakeholders_set_acked[i]=1;
							//cur_lock->stakeholders_locked[i]=1; //moved to lock_set_ack
							on_row[cur_lock->stakeholders[i]]=0;
							//cur_lock->stakeholders_released[i]=0; //set that back to unreleased, doing it here b/c, b/c, meh it made sense at the time
							if(cur_lock->stakeholders[i]==j)//skip ack message if local
							{	cur_lock->stakeholders_locked[i]=1;
								cur_lock->stakeholder_versions[i]= cur_lock->version; 	
								continue;
							}
							new_packet=malloc(sizeof(message));
							make_message(new_packet,j, LOCK_SET_ACK,packet->content,NULL); //notify of update
							add_query(nodes+cur_lock->stakeholders[i],new_packet);								
						}
					}
					sum_valid=0;
					for(i=0;i<cur_lock->num_stakeholders;i++)
					{	if(!cur_lock->stakeholders_set_acked[i])
							break;
						else
							sum_valid++;  
					}
					if(sum_valid>=cur_lock->num_stakeholders)
					{	flag=GRANTED;//GRANTED lock!! We've heard from all the stakeholders
						cur_lock->locked=1; 
						//for(i=0;i<cur_lock->num_stakeholders;i++)//done with the lock acks!
						//	cur_lock->stakeholders_set_acked[i]=0; 
					}
					else
					{	flag=PENDING;//PENDING... now we're going to go notify the rest of the stakeholders
						new_packet=malloc(sizeof(message));
						make_message(new_packet,packet->sender,SEND_LOCK_SET,packet->content,NULL); //send back a DENIED status for the lock request
						add_query(nodes+j,new_packet); 
					}
					
				}
				//if(cur_lock->holder==j)
				//	printf("the owner is asking for the lock...help!\n");
				if(packet->sender==j && flag != GRANTED)
				{	remove_query(nodes+j,packet);
					time=1; 
					break; 
				}
				else	//if(!(packet->sender==j && flag!=GRANTED))
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,packet->sender,flag,packet->content,NULL); //send back status of the lock request
					add_query(nodes+packet->sender,new_packet);
				}
				remove_query(nodes+j,packet);
			
			}
			time=1; 
			break; 
		case DENIED:	//BUSY
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			new_packet=malloc(sizeof(message));
			make_message(new_packet,j,LOCK_REQ,packet->content,NULL); //Keep asking... for now
			add_query(nodes+j,new_packet); 
			remove_query(nodes+j,packet);
			(nodes+j)->Acks++;
			(nodes+j)->Total++;
			time=1;
			break;
		case PENDING: //PENDING
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			//new_packet=malloc(sizeof(message));
			//make_message(new_packet,j,LOCK_REQ,packet->content,NULL); //Keep asking... for now
			//add_query(nodes+j,new_packet); 
			remove_query(nodes+j,packet);
			(nodes+j)->Acks++;
			(nodes+j)->Total++;
			time=1;
			break;
		case GRANTED: //GRANTED
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			//if(cur_lock->holder==cur_lock->owner)
			//	printf("here!\n");
			//Clear out stakeholder ack's so things don't get screwed up release
			//for(i=0;i<cur_lock->num_stakeholders;i++)
			//	cur_lock->stakeholders_acked[i]=0;
			//cur_lock->locked=1; 
			new_packet=malloc(sizeof(message));
			make_message(new_packet,j,LOCK_RELEASE,packet->content,NULL);//just here for testing purposes!!
			add_query(nodes+j,new_packet);//TODO: Take this out later!!!!!
			remove_query(nodes+j,packet);
			(nodes+j)->Acks++;
			(nodes+j)->Total++;
			time=1;
			break;
		case LOCK_SET_ACK: //sent to a node that heard lock set and replied
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			i=is_stakeholder(cur_lock,j);
			if(i<0)
				printf("Error!!!!\n");
			//Pull off any lock releases running around for the same holder
			cur_lock->stakeholder_versions[i]= cur_lock->version; //update version
			cur_lock->stakeholders_locked[i]=1;
			remove_lock_release_notice(nodes+j,packet->content->ID);
			remove_query(nodes+j,packet);
			(nodes+j)->Updates++;
			(nodes+j)->Total++;
			time=1;
			break;
		case SEND_LOCK_SET: //Hunting for ack's, but we don't report to lock requestor
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			//if(cur_lock->holder==j)
			//	printf("in send lock set\n");
			if(!cur_lock->locked)
			{	for(i=0;i<cur_lock->num_stakeholders;i++) //check if any of the stakeholders is on
				{	if(on_row[cur_lock->stakeholders[i]] && !cur_lock->stakeholders_set_acked[i]) //one of the stakeholders is on and it hasn't acked
					{	cur_lock->stakeholders_set_acked[i]=1;
						on_row[cur_lock->stakeholders[i]]=0;
						//cur_lock->stakeholders_released[i]=0;
						if(cur_lock->stakeholders[i]==j)//skip ack message if local
						{	cur_lock->stakeholders_locked[i]=1;
							cur_lock->stakeholder_versions[i]= cur_lock->version; 		
							continue;
						}
						new_packet=malloc(sizeof(message));
						make_message(new_packet,j, LOCK_SET_ACK,packet->content,NULL); //notify of update
						add_query(nodes+cur_lock->stakeholders[i],new_packet);	
					}
				}
			}
			sum_valid=0;
			for(i=0;i<cur_lock->num_stakeholders;i++)
			{	if(!cur_lock->stakeholders_set_acked[i])
					break;
				else
					sum_valid++; 
			}
			
			if(sum_valid<cur_lock->num_stakeholders) //Keep looking
			{	//new_packet=malloc(sizeof(message));
				//make_message(new_packet,packet->sender,SEND_LOCK_SET,packet->content,NULL); //send back a DENIED status for the lock request
				//add_query(nodes+j,new_packet);
				//remove_query(nodes+j,packet);
				flag++; 
			}
			else
			{	cur_lock->locked=1;
				if(on_row[packet->sender])
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,j,GRANTED,packet->content,NULL); //send back a DENIED status for the lock request
					add_query(nodes+cur_lock->holder,new_packet);
					remove_query(nodes+j,packet);
				}
			}
			time=1;
			break; 
		case LOCK_RELEASE: //Let it go, let it go can't hold it back anymore... 
			//if(is_stakeholder(cur_lock,j)!=0)//if it isn't a stakeholder, go find one
			owner=cur_lock->owner;
			if(owner!=j)//if not the owner, try to get in touch 
			{	if(total_time>0) //don't send anything if not enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				}
				flag=0; 
				time=1;
				//for(i=0;i<cur_lock->num_stakeholders;i++)
					if(on_row[owner]) //stakeholders[i]]==1 /*&& (!(nodes+cur_lock->stakeholders[i])->Queue || (nodes+cur_lock->stakeholders[i])->Queue->message_type<5 || (nodes+cur_lock->stakeholders[i])->Queue->content->ID==cur_lock->ID)*/) //some copyholder is on, TODO: randomize this!!
					{	new_packet=malloc(sizeof(message));
						make_message(new_packet,j,LOCK_RELEASE,packet->content,NULL); //need to throw 0 packet on there!!
						add_query(nodes+cur_lock->owner,new_packet); //I shouldn't be leaving "owner" here b/c it's not strictly correct...
						(nodes+j)->Hits+=1;
						(nodes+j)->Total+=1;
						(nodes+owner)->Acks++;
						(nodes+owner)->Total++; 
						on_row[owner]=0;
						remove_query(nodes+j,packet);
						flag=1;
						break;
					}
					else//meaning we didn't get a hold of the owner 
					{	(nodes+j)->Misses++;
						(nodes+j)->Total++;
					}	
			}
			else //is the owner, now has to go tell everybody else about lock...
			{	if(cur_lock->holder!=packet->sender) //if node other than owner tries to release lock, decline to aquiesce to the request... means no
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,j,DENIED,packet->content,NULL); //need to throw 0 packet on there!!
					add_query(nodes+packet->sender,new_packet); //I shouldn't be leaving "owner" here b/c it's not strictly correct...
					remove_query(nodes+j,packet);		
					free(copyholders);
					free(avail_row);
					return time; 
				}
				remove_lock_set(nodes+j,cur_lock->ID); //really shouldn't come back with anything
				cur_lock->locked=0;
				//(nodes+j)->release_acks=realloc((nodes+j)->release_acks,sizeof(int)*cur_lock->num_stakeholders);
				//if(j==cur_lock->owner)//clear holder if this is the owner... 
				cur_lock->holder=-1;
				for(i=0;i<cur_lock->num_stakeholders;i++)
					cur_lock->stakeholders_release_acked[i]=0; //clear out release acks to start counting fresh. 

				//Run through stakeholders and try to tell them it's released.
				for(i=0;i<cur_lock->num_stakeholders;i++) //check if any of the stakeholders is on
				{	if((on_row[cur_lock->stakeholders[i]] || cur_lock->stakeholders[i]==j) && !cur_lock->stakeholders_release_acked[i]) //one of the stakeholders is on and it hasn't been released
					{	on_row[cur_lock->stakeholders[i]]=0;
						cur_lock->stakeholders_release_acked[i]=1;
						if(cur_lock->stakeholders[i]==j)//skip ack message if local
						{	cur_lock->stakeholders_locked[i]=0;
							continue;
						}
						new_packet=malloc(sizeof(message));
						make_message(new_packet,j, LOCK_RELEASE_ACK,packet->content,NULL); //notify of update
						add_query(nodes+cur_lock->stakeholders[i],new_packet);			
					}
				}
				sum_valid=0;
				for(i=0;i<cur_lock->num_stakeholders;i++)
				{	if(!cur_lock->stakeholders_release_acked[i])
						break;
					else
						sum_valid++; 
				}
				if(sum_valid<cur_lock->num_stakeholders)
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,j,SEND_LOCK_RELEASE,packet->content,NULL); //send back a DENIED status for the lock request
					add_query(nodes+j,new_packet); 		
				}
				remove_query(nodes+j,packet);
			}
			time=1; //TODO: fix for when there is a single stakeholder!!!
			break; 
		case SEND_LOCK_RELEASE:
			/*if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}*/
			sum_valid=0;
			for(i=0;i<cur_lock->num_stakeholders;i++) //check if any of the stakeholders is on
			{	if(on_row[cur_lock->stakeholders[i]] && !cur_lock->stakeholders_release_acked[i]) //one of the stakeholders is on and it hasn't acked
				{	cur_lock->stakeholders_release_acked[i]=1;
					on_row[cur_lock->stakeholders[i]]=0;
					if(cur_lock->stakeholders[i]==j)//skip ack message if local
					{	cur_lock->stakeholders_locked[i]=0; 	
						continue;
					}
					new_packet=malloc(sizeof(message));
					make_message(new_packet,j, LOCK_RELEASE_ACK,packet->content,NULL); //notify of update
					add_query(nodes+cur_lock->stakeholders[i],new_packet);	
				}
			}
			for(i=0;i<cur_lock->num_stakeholders;i++)
			{	if(!cur_lock->stakeholders_release_acked[i])
					break;
				else
					sum_valid++; 
			}
			if(sum_valid<cur_lock->num_stakeholders) //Keep looking
			{	new_packet=malloc(sizeof(message));
				make_message(new_packet,j,SEND_LOCK_RELEASE,packet->content,NULL); //send back a DENIED status for the lock request
				add_query(nodes+j,new_packet);
				remove_query(nodes+j,packet);
			}
			else
				remove_query(nodes+j,packet);
			time=1; //TODO: fix for when there's only a single stakeholder and this is the owner!!
			break;
		case LOCK_RELEASE_ACK:
			if(total_time>0)//--> again, REALLY shouldn't happen
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			i=is_stakeholder(cur_lock,j);
			flag=is_stakeholder(cur_lock,packet->sender);
			if(i<0)
				printf("Error!!!!\n");
			if(cur_lock->stakeholder_versions[i] > cur_lock->stakeholder_versions[flag]) //really shouldn't happen w/ single lock arbiter
			{	remove_lock_release_notice(nodes+packet->sender,packet->content->ID);
				remove_query(nodes+j,packet);
				time=1;
				break; 
			}
			//if(j==cur_lock->owner)//clear holder & stakeholder acks if this is the owner... 
			//	cur_lock->holder=-1;
			cur_lock->stakeholders_locked[i]=0;
			
			//if(packet)
				remove_query(nodes+j,packet);
			time=1; 
			break; 
		default: 
			printf("Something screwed up really badly!!!\n");
			exit(0);
	}
	free(copyholders);
	free(avail_row);
	//if(cur_lock->holder!=-1 && !cur_lock->locked)
	//	printf("Why??????????\n");
	return time;
	
}




