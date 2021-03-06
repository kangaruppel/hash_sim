#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct data_{
	int ID;
	int owner;
	int rep_factor;
	int invalid_accesses;
	int num_writers;
	int *writers;
	int *copyholders;
	int *valid_copies;
	int *invalid_time_start;
	int *invalid_time_total;
}data;

typedef struct message_{ 
	int message_type;
	data *content;
	struct message_ *next;
}message;

typedef struct node_{
	int ID;
	message *Queue;
	int Updates;
	int Hits;
	int Misses;
	int Acks;
	int Total;
}node;

int make_node(node *, int, message *);
int add_query(node *, message *);
int remove_query(node *, message *);
void print_queue(node *);
int build_node_arr(node *, int, int, int, float, data *);

int make_message(message *, int, data *, message *);
message * build_request_list(int, int, data *, float);

int make_data(data *,int, int,int);
int * copyfinder(int, int, int);
int pass_out_data(data *, int, int, int);

long process_requests(int **, node *, data *, int, int, int);
float read_off_queue(int, data *, node *, int *, double, float, int);
int all_queues_empty(node *, int);
float staleness_checker(data *, long *, int);
void shuffle(int *, size_t );

int RETRY_LEVEL;
int WRITER_LEVEL;