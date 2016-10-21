#include "hash_sim.h"

//This function takes a pointer to a node, and ID number and a pointer 
//to a linked list of messages as input and initializes the node. 
int make_node(node *new_node, int ID, message *requests)
{	//new_node=malloc(sizeof(node));
	if(!new_node)
		return -1;
	new_node->ID=ID;
	new_node->Queue=requests;
	new_node->Updates=0;
	new_node->Hits=0;
	new_node->Misses=0;
	new_node->Acks=0;
	new_node->Total=0;
	return 0; 
}

int free_node(node *node_in)
{	message *cur, *prev; 
	prev=NULL;
	for(cur=node_in->Queue;cur;cur=cur->next)
	{	if(prev)
			free(prev);
		prev=cur;
	}
	free(prev);

	return 0; 
}

//Perform head insertion of new request into request queue
int add_query(node *input,message *new_request)
{	//printf("Adding %i %i %i to %i\n", new_request->content->ID, new_request->content->owner, new_request->message_type, input->ID);
	new_request->next=input->Queue; 
	input->Queue=new_request; 	
	return 0; 
}

//Remove head of request queue list and frees that memory
int remove_query(node *input,message *remove)
{	message *trash, *cur, *prev; 
	//printf("Removing %i %i %i from %i\n",remove->content->ID,remove->content->owner, remove->message_type, input->ID);
	cur=input->Queue;
	prev=input->Queue;
	if(remove==input->Queue) //head of list
	{	trash=input->Queue;
		input->Queue=input->Queue->next;
	}
	else
	{	for(cur=input->Queue;cur!=remove && cur->next;cur=cur->next)
			prev=cur;
		if(cur!=remove)
			return -1;
		prev->next=cur->next;
		trash=cur;
	}

	free(trash);
	return 0; 
}

//Print out the contents of a node's queue
void print_queue(node *input)
{	message *cur; 
	for(cur=input->Queue; cur; cur=cur->next) 
		printf("%5d \t %5d \n", cur->content->ID, cur->message_type);
	
	return;
}

//Initialize a message
int make_message(message *input, int type, data *stuff, message *next)
{	//input=malloc(sizeof(message));
	if(!input)
		return -1;
	if(type>4 || type<0)
	{	printf("Error!");
		return -1;
	}
	input->message_type=type;
	input->content=stuff;
	input->next=next;
	return 0; 
}

//Initialize a piece of data
int make_data(data *input, int ID, int num_nodes, int rep_factor)
{	int i;
	//input=malloc(sizeof(data));
	if(!input)
		return -1;
	input->ID=ID;
	input->owner=ID % num_nodes; 
	input->rep_factor=rep_factor;
	input->invalid_accesses=0;
	input->copyholders=copyfinder(num_nodes,rep_factor,input->owner);
	input->valid_copies=malloc(sizeof(int)*rep_factor);
	input->invalid_time_start=malloc(sizeof(int)*rep_factor);
	input->invalid_time_total=malloc(sizeof(int)*rep_factor);
	if(!input->copyholders || !input->invalid_time_start || !input->valid_copies || !input->invalid_time_total)
		return -1;
	for(i=0;i<rep_factor;i++)
	{	input->valid_copies[i]=1;
		input->invalid_time_start[i]=0;
		input->invalid_time_total[i]=0;
	}
	return 0;
}

int free_data(data *data_in)
{	free(data_in->copyholders);
	free(data_in->valid_copies);
	free(data_in->invalid_time_start);
	free(data_in->invalid_time_total);
	return 0; 
}
int *copyfinder(int num_nodes, int rep_factor, int owner)
{	int i;
	int *copies=malloc(sizeof(int)*(rep_factor-1));
	if(!copies)
		exit(0);
	for(i=0;i<rep_factor-1;i++)
	{	if(owner-1>=0) //check for corner case
			owner=owner-1;
		else
			owner=num_nodes-1; //roll back to the top
		copies[i]=owner;
	}
	return copies;
}


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
				printf("%i ", k);
			}	
			//if(avail_row[k])
			//	printf("found one!!\n");
		}
		if(on_index>1)
			shuffle(nodes_on,on_index);
		for(k=0;k<on_index;k++)
			printf("%i ",nodes_on[k]);
		num_tried=0;
		//print_queue(nodes+3);
		//printf("\n");
		on=0;
		printf("\nStart a j:\n");
		while(num_tried<num_on)
		{	j=nodes_on[num_tried]; //pick a random start point, also shouldn't have to worry about cleaning out this array every time...
			total_time=0;
			//num_tried++;
			printf("j=%i\n",j);
			if((nodes+j)->Queue && on_mat[i][j])
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
		if(num_on>0)
			printf("Tried all nodes\n");
		/*if(on)
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
	for(i=0;i<time_to_finish;i++)
		printf("%f\n", stale_timelog[i]);
	printf("Cumulative staleness per variable\n");
	for(i=0;i<namespace_size;i++)
		printf("%ld\n",stale_vector[i]);
	printf("\n");
	//fclose(output_file);
	free(avail_row);
	free(stale_vector);
	free(stale_timelog);
	return time_to_finish;
		
}


float read_off_queue(int j, data *data_arr, node *nodes, int *on_row, double  global_time, float total_time, int num_nodes)
{	//printf("J=%i\n",j);
	message *packet=(nodes+j)->Queue; //Later right scheduler function to pack in with message type!!!
	if(!packet)
		return 1; 
	message *new_packet, *test_packet;
	data *dummy;
	int ID=packet->content->ID, rep_factor=packet->content->rep_factor;
	int *copyholders=malloc(sizeof(int)*packet->content->rep_factor);
	int owner=packet->content->owner;
	int *avail_row=malloc(sizeof(int)*num_nodes);
	int i, flag=1, sum_valid=0, extra;
	float time=0; 
	for(i=0;i<packet->content->rep_factor-1;i++)
		copyholders[i]=packet->content->copyholders[i];
	for(i=0;i<num_nodes;i++)
		avail_row[i]=on_row[i];
	switch(packet->message_type)
	{	case 0: //Schedule a write
			if(owner==j) //it's local & we have write permission!
			{	time=.01;
				//remove_query(nodes+j,packet);
				(nodes+j)->Hits+=1;
				(nodes+j)->Total+=1;
				(data_arr+ID)->valid_copies[0]=1;
				(data_arr+ID)->invalid_time_start[0]=0;
				(data_arr+ID)->invalid_time_total[0]=0;
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
				sum_valid=1;
				for(i=1;i<rep_factor;i++)
				{	(data_arr+ID)->valid_copies[i]=0; //Zero out valid trackers
					(data_arr+ID)->invalid_time_start[i]=global_time;
					if(on_row[copyholders[i-1]] && flag) //one of the copies is on and there's time left
					{	sum_valid++;
						new_packet=malloc(sizeof(message));
						make_message(new_packet,2,packet->content,NULL); //notify of update
						add_query(nodes+copyholders[i-1],new_packet);
						//avail_row[copyholders[i-1]]=0;
						on_row[copyholders[i-1]]=0; //<-- replaced line above as a test.... 10-16-16
						(data_arr+ID)->valid_copies[i]=1;
						(data_arr+ID)->invalid_time_total[i]=0; //since it was on when the owner first issued a write
						(data_arr+ID)->invalid_time_start[i]=0; //reset
					}
					
				}
					
				if(sum_valid<rep_factor) //Add packet back on
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,4,packet->content,NULL);
					remove_query(nodes+j,packet);
					add_query(nodes+j,new_packet);
				}
								
			}
			else //need to tell owner to update
			{	if(total_time>0) //don't send anything if not enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				}
				time=1;
				if(on_row[owner]==1) //owner is on
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,0,packet->content,NULL); //need to throw 0 packet on there!!
					add_query(nodes+owner,new_packet);
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
			for(i=0;i<rep_factor-1;i++)
			{	if(copyholders[i]==j)
				{	flag=1; //local copy
					break;
				}
			}
			if(owner==j)
				flag=1;
			if(flag) //Local read
			{	time=.01;
				remove_query(nodes+j,packet);
				(nodes+j)->Hits++;
				(nodes+j)->Total++;
			}
			else
			{	if(total_time>0) //Quit if there isn't enough time
				{	free(avail_row);
					free(copyholders);
					return 1;
				}
				time=1;
				flag=0;
				for(i=1;i<rep_factor;i++) //check for any copyholders that are on. 
				{	if(on_row[copyholders[i-1]])
					{	new_packet=malloc(sizeof(message));
						make_message(new_packet,3,packet->content,NULL); //notify of update
						add_query(nodes+copyholders[i-1],new_packet);
						//avail_row[copyholders[i-1]]=0;
						on_row[copyholders[i-1]]=0;
						remove_query(nodes+j,packet);
						(nodes+j)->Hits++;
						(nodes+j)->Total++;
						flag=1;
						break;
					}
				}
				if(!flag)
				{	if(on_row[owner])//check if the owner is on
					{	new_packet=malloc(sizeof(message));
						make_message(new_packet,3,packet->content,NULL); //notify of update
						add_query(nodes+owner,new_packet);
						//avail_row[owner]=0;
						on_row[owner]=0;
						remove_query(nodes+j,packet);
						(nodes+j)->Hits++;
						(nodes+j)->Total++;
					}
					else
					{	(nodes+j)->Misses++;
						(nodes+j)->Total++;		
					}
					
				}
			}
			break;
		case 2: //scheduling consistency update
			if(total_time>0)
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
			if(total_time>0)
			{	free(avail_row);
				free(copyholders);
				return 1;
			}
			if(packet->content->owner==j)//this node is the data owner
				i=0;
			else
				for(i=0;i<rep_factor-1, copyholders[i]!=j;i++);	//run through copies and find the index of this node
			if(!packet->content->valid_copies[i])	
				packet->content->invalid_accesses++;
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
			//remove_query(nodes+j,packet);
			(nodes+j)->Updates++;
			(nodes+j)->Total++;
			time=1;
			(data_arr+ID)->valid_copies[0]=1;
			(data_arr+ID)->invalid_time_start[0]=0;
			(data_arr+ID)->invalid_time_total[0]=0;
			for(i=1;i<rep_factor;i++)
			{	if(on_row[copyholders[i-1]] && (data_arr+ID)->valid_copies[i]==0)
				{	new_packet=malloc(sizeof(message));
					make_message(new_packet,2,packet->content,NULL); //notify of update
					add_query(nodes+copyholders[i-1],new_packet);
					//avail_row[copyholders[i-1]]=0;
					on_row[copyholders[i-1]]=0;
					(data_arr+ID)->valid_copies[i]=1;
					(data_arr+ID)->invalid_time_total[i]=global_time-(data_arr+ID)->invalid_time_start[i]; //clock in the total time it was inconsistent
					(data_arr+ID)->invalid_time_start[i]=0; //reset		
				}
			}
			sum_valid=0;
			for(i=0;i<rep_factor;i++)
				sum_valid+=(data_arr+ID)->valid_copies[i];
			if(sum_valid>=rep_factor)
				remove_query(nodes+j,packet);
			/*{	new_packet=malloc(sizeof(message));
				extra=(nodes+j)->ID;
				test_packet=new_packet;
				dummy=packet->content;
				make_message(new_packet,4,packet->content,NULL);
				extra=new_packet->content->ID;
				add_query(nodes+j,new_packet);
			}*/
			break;
		default: 
			printf("Something screwed up really badly!!!\n");
			exit(0);
	}
	free(copyholders);
	free(avail_row);
	return time;
	
}