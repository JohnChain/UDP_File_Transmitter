#include"heads.h"
#include"conf.h"

#define NEW 1
#define OLD 0

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

int task_flag; //judge this download is NOW or OLD 
int downloaded_block;
int thread_num;
int blocksize;
int block_num;
long filesize;
char filename[LEN_FILENAME];

char conf_filename[LEN_FILENAME];

pthread_t ptid[THREAD_LIMITION];

pthread_mutex_t down_lock = PTHREAD_MUTEX_INITIALIZER;


FILE *tfd;
char buf[8192];
