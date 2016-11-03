#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ALL 1
#define COPIES 2

typedef struct lock_{
	int locked;
	int ID;
	int owner;
	int holder;
	int *stakeholders;
}lock;

typedef struct data_{
	int ID;
	int owner;
	int rep_factor;
	int invalid_accesses;
	int num_writers;
	int *copyholders;
	int *mod_times;
	int *valid_copies;
	int *invalid_time_start;
	int *on_while_invalid_cnt;
	int *time_on_and_invalid; 
	int *invalidated_cnt;
	int *last_onoff_state;//0=off, 1=on
	int *last_valid_state;//0=stale, 1=valid
	float *avg_time_on_while_invalid;
	float *avg_time_invalid; 
	lock *data_lock; 
}data;



typedef struct write_{
	int ID;
	int version;
	int *copies_acked;
	struct write_ *next;
}write;

typedef struct message_{ 
	int message_type;
	data *content;
	int sender;
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
	write *Active_writes;
}node;

int make_node(node *, int, message *);
int add_query(node *, message *);
int remove_query(node *, message *);
void print_queue(node *);
int build_node_arr(node *, int, int, int, float, data *);

int make_message(message *, int,int, data *, message *);
message * build_request_list(int, int, data *, float,int);

int make_write(write *, int, int, int);
int add_write(node *, write *);
int remove_write(node *, write *);

int make_data(data *,int, int,int);
int * copyfinder(int, int, int);
int pass_out_data(data *, int, int, int);
int is_copyholder(data *input, int dut);
//int is_writer(data *input, int dut);

long process_requests(int **, node *, data *, int, int, int);
float read_off_queue(int, data *, node *, int *, double, float, int);
int all_queues_empty(node *, int);
float staleness_checker(data *, int, int, int *);
void shuffle(int *, size_t );
int new_write_monitoring(int j, node *nodes, data *content, double global_time);

int RETRY_LEVEL;
int MULTI_WRITER;
int FINISH_ALL_UPDATES;
int NEWER_WRITE_NOT_REQ;
FILE *STALE_OUTPUT_FILE; 