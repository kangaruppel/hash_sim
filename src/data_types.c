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
	new_node->Active_writes=NULL;
	return 0; 
}

int free_node(node *node_in)
{	message *cur, *prev; 
	write *cur2, *prev2;
	prev=NULL;
	for(cur=node_in->Queue;cur;cur=cur->next)
	{	if(prev)
			free(prev);
		prev=cur;
	}
	prev2=NULL;
	for(cur2=node_in->Active_writes;cur2;cur2=cur2->next)
	{	if(prev2)
			free(prev2);
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
int make_message(message *input,int sender, int type, data *stuff, message *next)
{	//input=malloc(sizeof(message));
	if(!input)
		return -1;
	if(type>4 || type<0)
	{	printf("Error!");
		return -1;
	}
	input->sender=sender; 
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
	input->num_writers=1;
	input->copyholders=copyfinder(num_nodes,rep_factor,input->owner);
	input->mod_times=malloc(sizeof(int));
	input->valid_copies=malloc(sizeof(int)*rep_factor);
	input->invalid_time_start=malloc(sizeof(int)*rep_factor);
	input->invalid_time_total=malloc(sizeof(int)*rep_factor);
	//input->mode_times=malloc(sizeof(int)*rep_factor);
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
	free(data_in->mod_times);
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

	free(trash);
	return 0; 
}

//This function sets up the new write monitoring node... It has j as and index into the 
//array, 
int new_write_monitoring(int j, node *nodes, data *content, double global_time)
{	write *new=malloc(sizeof(write));
	(nodes+j)->Hits+=1;
	(nodes+j)->Total+=1;
	content->num_writers++; //Basically increment the most up-to-date version number
	if(!new)
		return -1;
	make_write(new,content->ID, content->num_writers, content->rep_factor);
	add_write(nodes+j, new);
	content->mod_times=realloc(content->mod_times,sizeof(int)*content->num_writers);
	content->mod_times[content->num_writers-1]=global_time;
	return 0; 
}
