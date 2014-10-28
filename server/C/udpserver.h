#ifndef HEAD_H
#define HEAD_H

#include "heads.h"
#include "queue.h"

#define YES 1
#define NO  0

#define NEW 1
#define OLD 0
/*#define DEFAULT_THREAD_NUM 4*/
#define SOCK_TIMEOUT_SEC 2
#define SOCK_TIMEOUT_USEC 0
#define RESENDTIME      10


extern void * mission(void * task);
extern void do_print(int thread_no, char *msg);
extern void * writer();

struct subthread_task
{
    int port;
    int thread_num;
    int thread_no;
    struct local_file file_info;
};

int downloaded_block;

/*pthread_mutex_t lock_cost_recv  = PTHREAD_MUTEX_INITIALIZER;*/
/*pthread_mutex_t lock_cost_write = PTHREAD_MUTEX_INITIALIZER;*/
/*pthread_mutex_t lock_cost_pack  = PTHREAD_MUTEX_INITIALIZER;*/
/*pthread_mutex_t lock_cost_send  = PTHREAD_MUTEX_INITIALIZER;*/

pthread_mutex_t lock_cost_recv  ;
pthread_mutex_t lock_cost_write ;
pthread_mutex_t lock_cost_pack  ;
pthread_mutex_t lock_cost_send  ;
float cost_recv;
float cost_write;
float cost_pack;
float cost_send;

QUEUE *data_queue;
int writer_live ;
#endif
