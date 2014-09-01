#include"heads.h"

#define YES 1
#define NO  0
#define DEFAULT_THREAD_NUM 4
#define SOCK_TIMEOUT_SEC 2
#define SOCK_TIMEOUT_USEC 0
#define RESENDTIME      20


extern void * mission(void * task);
extern void do_print(int thread_no, char *msg);

struct subthread_task
{
    int port;
    int thread_num;
    int thread_no;
    struct local_file file_info;
};

int downloaded_block;
