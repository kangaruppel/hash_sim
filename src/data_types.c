#include "hash_sim.h"

int make_lock(lock *new_lock,int ID, int owner, int rep_factor, int *copyholders)
{	int i; 
	new_lock->locked=0; 
	new_lock->num_stakeholders=rep_factor;
	new_lock->ID=ID;
	new_lock->owner=owner;
	new_lock->holder=-1;
	new_lock->version=-1; 
	new_lock->stakeholders_set_acked=malloc(sizeof(int)*rep_factor);
	new_lock->stakeholders_release_acked=malloc(sizeof(int)*rep_factor);
	new_lock->stakeholders_locked=malloc(sizeof(int)*rep_factor);
	new_lock->stakeholder_versions=malloc(sizeof(int)*rep_factor); 
	new_lock->stakeholders=copyholders;
	for(i=0;i<rep_factor;i++)
	{	new_lock->stakeholders_locked[i]=0;
		new_lock->stakeholders_set_acked[i]=0;
		new_lock->stakeholders_release_acked[i]=0;
		new_lock->stakeholder_versions[i]=-1;
	}
	if(!new_lock->stakeholders)
		return -1;
	return 0; 
}

int free_lock(lock *lock_in)
{	free(lock_in->stakeholders);
	free(lock_in->stakeholders_set_acked);
	free(lock_in->stakeholders_release_acked);
	free(lock_in->stakeholders_locked);
	free(lock_in->stakeholder_versions);
	free(lock_in); 
	return 0; 
}


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
	//new_node->lock_acks=malloc(sizeof(int ));
	//new_node->release_acks=malloc(sizeof(int ));
	new_node->Active_writes=NULL;
	return 0; 
}

int free_node(node *node_in)
{	message *cur, *prev; 
	write *cur2, *prev2;
	//free(node_in->lock_acks);
	//free(node_in->release_acks);
	prev=NULL;
	for(cur=node_in->Queue;cur;cur=cur->next)
	{	if(prev)
			free(prev);
		prev=cur;
	}
	prev2=NULL;
	for(cur2=node_in->Active_writes;cur2;cur2=cur2->next)
	{	if(prev2)
			free_write(prev2);
		prev2=cur2;
	}
	free(prev2);
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

//pos=0 && relative!=NULL : add after relative
//pos=1 && relative!=NULL : add before relative
//relative==NULL, add at the number'th' position
int add_query_special(node *input,message *new_request,message *relative, int pos, int number)
{	message *cur, *prev; 
	int i; 
	if(!relative)//adding by position
	{	if(number==0)
			add_query(input,new_request); //standard head insertion
		else
		{	for(cur=input->Queue, i=0; i<number && cur; i++, cur=cur->next)
				prev=cur;
			prev->next=new_request;
			new_request->next=cur; 
		} 
	}
	else//adding based on another node in the queue
	{	if(relative==input->Queue)
		{	if(pos)//adding before head
			{	new_request->next=input->Queue;
				input->Queue=new_request;
			}
			else//adding after head
			{	new_request->next=input->Queue->next;
				input->Queue->next=new_request; 
			}
		}
		else
		{	for(cur=input->Queue;cur!=relative && cur->next;cur=cur->next)
				prev=cur;
			if(cur!=relative)
				return -1;
			if(pos)//adding before
			{	new_request->next=cur;
				prev->next=new_request; 
			}
			else//adding after
			{	new_request->next=cur->next;
				cur->next=new_request;
			}
		}	
	}
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

//Find a pending lock release query to a certain value and get rid of it
//since the value was locked again. 
int remove_lock_release_notice(node *input,int ID)
{	message *trash, *cur, *prev; 
	trash=NULL; 
	cur=NULL;
	prev=NULL; 
	//printf("Removing %i %i %i from %i\n",remove->content->ID,remove->content->owner, remove->message_type, input->ID);
	cur=input->Queue;
	prev=input->Queue;
	if(!input->Queue)
		return 0; 
	if(input->Queue->content->ID == ID &&(input->Queue->message_type== SEND_LOCK_RELEASE || input->Queue->message_type== LOCK_RELEASE)) //head of list
	{	trash=input->Queue;
		input->Queue=input->Queue->next;
	}
	else
	{	for(cur=input->Queue;cur; cur=cur->next)
		{	if((cur->message_type==SEND_LOCK_RELEASE || cur->message_type== LOCK_RELEASE) && cur->content->ID==ID)	
				break; 
			prev=cur;
		}
		if(cur)
		{	prev->next=cur->next;
			trash=cur;
		}
	}
	if(trash)
		free(trash);
	return 0; 
}

//Find a pending lock set query to a certain value and get rid of it
//since a set packet will be added again. 
int remove_lock_set(node *input,int ID)
{	message *trash, *cur, *prev; 
	trash=NULL; 
	cur=NULL;
	prev=NULL; 
	//printf("Removing %i %i %i from %i\n",remove->content->ID,remove->content->owner, remove->message_type, input->ID);
	cur=input->Queue;
	prev=input->Queue;
	if(input->Queue->content->ID == ID &&(input->Queue->message_type==SEND_LOCK_SET)) //head of list
	{	trash=input->Queue;
		input->Queue=input->Queue->next;
	}
	else
	{	for(cur=input->Queue;cur; cur=cur->next)
		{	if((cur->message_type==SEND_LOCK_SET) && cur->content->ID==ID)	
				break; 
			prev=cur;
		}
		if(cur)
		{	prev->next=cur->next;
			trash=cur;
		}
	}
	if(trash)
		free(trash);
	return 0; 
}


//Print out the contents of a node's queue
void print_queue(node *input,int quantity)
{	int i; 
	message *cur; 
	if(!quantity && input->Queue)
	{	for(cur=input->Queue; cur; cur=cur->next) 
			printf("%5d \t %5d \n", cur->content->ID, cur->message_type);
	}
	else
	{	if(input->Queue)
		{	for(i=0,cur=input->Queue; i<quantity && cur; i++,cur=cur->next) 
				printf("%5d \t %5d \n", cur->content->ID, cur->message_type);
		}
	}
	return;
}

void print_all_queues(node *nodes, int quantity)
{	 int i; 
	for(i=0;i<25;i++)
	{	printf("\n%i\n",i);
		print_queue(nodes+i,quantity);
	
	}
	return; 
}
	
	
	


//Initialize a message
int make_message(message *input,int sender, int type, data *stuff, message *next)
{	//input=malloc(sizeof(message));
	if(!input)
		return -1;
	input->sender=sender; 
	input->message_type=type;
	input->content=stuff;
	input->next=next;
	return 0; 
}

//Initialize a piece of data
int make_data(data *input, int ID, int num_nodes, int rep_factor)
{	int i;
	lock *new_lock; 
	new_lock=malloc(sizeof(lock));
	//input=malloc(sizeof(data));
	if(!input)
		return -1;
	input->ID=ID;
	input->owner=ID % num_nodes; 
	input->rep_factor=rep_factor;
	input->invalid_accesses=0;
	input->num_writers=0;
	input->risk_window_hits=0;
	input->total_reads=0; 
	input->copyholders=copyfinder(num_nodes,rep_factor,input->owner);
	input->mod_times=malloc(sizeof(int));
	input->valid_copies=malloc(sizeof(int)*rep_factor);
	input->invalid_time_start=malloc(sizeof(int)*rep_factor);
	input->on_while_invalid_cnt = malloc(sizeof(int)*rep_factor);
	input->time_on_and_invalid = malloc(sizeof(int)*rep_factor);
	input->invalidated_cnt =malloc(sizeof(int)*rep_factor);
	input->last_onoff_state = malloc(sizeof(int)*rep_factor); 
	input->last_valid_state = malloc(sizeof(int)*rep_factor); 
	input->avg_time_on_while_invalid = malloc(sizeof(float)*rep_factor);
	input->avg_time_invalid = malloc(sizeof(float)*rep_factor);
	//input->mode_times=malloc(sizeof(int)*rep_factor);
	if(!input->copyholders || !input->invalid_time_start || !input->valid_copies )
		return -1;
	for(i=0;i<rep_factor;i++)
	{	input->valid_copies[i]=0;
		input->invalid_time_start[i]=0;
		input->on_while_invalid_cnt[i]=0;
		input->time_on_and_invalid[i]=0;
		input->invalidated_cnt[i]=0;
		input->last_onoff_state[i]=0;
		input->last_valid_state[i]=1;
		input->avg_time_invalid[i]=0;
		input->avg_time_on_while_invalid[i]=0;
	}
	make_lock(new_lock,ID,input->owner,rep_factor, input->copyholders);
	input->data_lock=new_lock; 
	return 0;
}

int free_data(data *data_in)
{	free(data_in->copyholders);
	free(data_in->valid_copies);
	free(data_in->invalid_time_start);
	if(data_in->mod_times)
		free(data_in->mod_times);
	free(data_in->on_while_invalid_cnt);
	free(data_in->time_on_and_invalid);
	free(data_in->invalidated_cnt);
	free(data_in->avg_time_invalid);
	free(data_in->avg_time_on_while_invalid);
	free(data_in->last_onoff_state);
	free(data_in->last_valid_state);
	free_lock(data_in->data_lock);
	return 0; 
}
int *copyfinder(int num_nodes, int rep_factor, int owner)
{	int i;
	int *copies=malloc(sizeof(int)*(rep_factor));
 	if(!copies)
		exit(0);
	copies[0]=owner;
	for(i=1;i<rep_factor;i++)
	{	if(owner-1>=0) //check for corner case
			owner=owner-1;
		else
			owner=num_nodes-1; //roll back to the top
		copies[i]=owner;
	}
	return copies;
}

//returns the index in the copyholder array of the device under test
//or -1 if it isn't in the array. 
int is_copyholder(data *input, int dut)
{	int i; 
	for(i=0;i<input->rep_factor;i++)
	{	if(dut==input->copyholders[i])
			return i;
	}
	return -1; 
}

int is_stakeholder(lock *input, int dut)
{	int i; 
	for(i=0;i<input->num_stakeholders;i++)
	{	if(dut==input->stakeholders[i])
			return i; 
	}
	return -1;
}
/*int is_writer(data *input, int dut)
{	int i; 
	for(i=0;i<input->num_writers;i++)
	{	if(dut==input->writers[i])
			return i; 
	}
	return -1;
}
*/

int make_write(write *input, int ID, int version, int rep_factor)
{	int i; 
	input->ID=ID;
	input->version=version;
	input->copies_acked=malloc(sizeof(int)*rep_factor);
	input->next=NULL;
	for(i=0;i<rep_factor;i++)
		input->copies_acked[i]=0; 
	return 0; 
}



//Perform head insertion of new write into list of ongoing writes. 
int add_write(node *input,write *new_request)
{	//printf("Adding %i %i %i to %i\n", new_request->content->ID, new_request->content->owner, new_request->message_type, input->ID);
	write *cur;
	message *cur2,  *temp;
	if(!new_request)
		printf("Add write failed!!\n");
	for(cur=input->Active_writes;cur->ID!=new_request->ID, cur; cur=cur->next);
	if(cur)
	{	remove_write(input,cur);
		for(cur2=input->Queue;cur2->next;cur2=temp)
		{	temp=cur2->next;
			if(cur2->message_type==4 && cur2->content->ID == new_request->ID) //If you find an update order from a stale copy, get that out of there!
				remove_query(input,cur2);
		}
	}
	new_request->next=input->Active_writes; 
	input->Active_writes=new_request; 	
	return 0; 
}

//Remove from list of writes and free that memory
//Should be using ADT here and reusing the queue, but meh, so it goes
int remove_write(node *input,write *remove)
{	write *trash, *cur, *prev; 
	//printf("Removing %i %i %i from %i\n",remove->content->ID,remove->content->owner, remove->message_type, input->ID);
	cur=input->Active_writes;
	prev=input->Active_writes;
	if(remove==input->Active_writes) //head of list
	{	trash=input->Active_writes;
		input->Active_writes=input->Active_writes->next;
	}
	else
	{	for(cur=input->Active_writes;cur!=remove && cur->next;cur=cur->next)
			prev=cur;
		if(cur!=remove)
			return -1;
		prev->next=cur->next;
		trash=cur;
	}

	free_write(trash);
	return 0; 
}

int free_write(write *input)
{	free(input->copies_acked);
	free(input);
	return 0; 
}

//This function sets up the new write monitoring node... It has j as and index into the 
//array, 
int new_write_monitoring(int j, node *nodes, data *content, double global_time)
{	int i;
	write *new=malloc(sizeof(write));
	(nodes+j)->Hits+=1;
	(nodes+j)->Total+=1;
	content->num_writers++; //Basically increment the most up-to-date version number
	for(i=0;i<content->rep_factor;i++)
		content->invalid_time_start[i]=global_time;
	if(!new)
		return -1;
	make_write(new,content->ID, content->num_writers, content->rep_factor);
	add_write(nodes+j, new);
	content->mod_times=realloc(content->mod_times,sizeof(int)*content->num_writers);
	content->mod_times[content->num_writers-1]=global_time;
	return 0; 
}
