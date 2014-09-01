#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/types.h>
/*#include<readline/readline.h>*/
/*#include<readline/history.h>*/
#include<errno.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/stat.h>
#include<signal.h>

#define YES 1
#define NO 0

#define BODYLEN 10240
#define MAXLEN  14480
#define SERV_PORT 8888

#define THREAD_LIMITION 10
#define LEN_FILENAME    32

#define TYPE_NUM    20
#define SYN         1
#define ACK         2
#define SYN_ACK     3
#define FIN         4
#define DATA        10

extern void init_protocal_buf();
struct message pack_msg(int type, int pack_no, int length, void *body);
extern struct message depack_msg(char *buf, int buf_len);
extern void print_msg_info(struct message *msg);
extern size_t get_filesize(int fd);
extern void print_process(int percent, int barlen);

extern int SYN_solution(int sockfd, struct message *msg, struct sockaddr pservaddr);
extern int ACK_solution(int sockfd, struct message *msg, struct sockaddr pservaddr);
extern int FIN_solution(int sockfd, struct message *msg, struct sockaddr pservaddr);
extern int DATA_solution(int sockfd, struct message *msg, struct sockaddr pservaddr);


struct message
{
    char szteam[6];
    int type;
    int pack_no;
    int length;
    char body[BODYLEN];
    unsigned char checkbyte;
};


struct local_file{
    char filename[LEN_FILENAME];
    int filesize;

    int breakpoint;
    int end_point;

    int blocksize;
    int block_num;
    int downloaded_block;
};

struct synack
{
    int downloadable;
    int thread_num;
    int port[THREAD_LIMITION];
    struct local_file file_info;
};

// 协议栈 
extern int (*protocal_buf[TYPE_NUM])(int , struct message *, struct sockaddr);
extern struct synack;
