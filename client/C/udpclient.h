#include"heads.h"
#include"conf.h"
#include"queue.h"
#include <dirent.h>

#define OLD_TASK 0
#define NEW_TASK 1

#define NEW     0
#define PENDING 1
#define ABANDON 2
#define DONE    3

#define SOCK_TIMEOUT_SEC 2
#define SOCK_TIMEOUT_USEC 0

struct subthread_task
{
    char *hostname;
    int port;

    int thread_no;
    int thread_num;

    struct local_file file_info;
};

int clientLive  ;
int soldierLive ;
int cookLive    ;
int receiverLive;

int task_flag; //judge this download is NOW or OLD 
int taskIndex; //global index share by all subthreads

char conf_file[LEN_FILENAME];
char file_name[LEN_FILENAME];
char path[500];
//char path[] = "/data/media/diag_logs/QXDM_logs/2014_10_11_04_46_24/";

int downloaded_block ;
int block_num        ;
int filesize         ;
char filename[LEN_FILENAME];

pthread_t ptid[THREAD_LIMITION];
QUEUE *queue ;
int *taskArray ;
struct subthread_task *task_list ;

pthread_mutex_t lockDownload = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockIndex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockSoldier = PTHREAD_MUTEX_INITIALIZER;

extern void store_detail(config_t *myconf);
extern void load_detail(config_t *myconf);
extern void *receiver(void *arg);
extern void init_env();
