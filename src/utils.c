/**
 * This file includes some public function used by all othre modules.
 * */
#include"heads.h"

int (*protocal_buf[TYPE_NUM])(int , struct message *, struct sockaddr);

struct message pack_msg(int type, int pack_no, int length, void *body)
{
    ///构造通信协议包
    //type 消息类型
    //length body字段长度
    //body 文件内容
    struct message msg;
    strcpy(msg.szteam, "SZTEAM");
    msg.type = type;
    msg.length = length;
    msg.pack_no = pack_no;
    //char *temp_buf= (char*)malloc(sizeof(char) * length);
    memcpy(msg.body, body, length);

    //printf("[in pack msg] synack size = %d, length = %d\n", sizeof(msg.body), length);

    //init_crc8();
    unsigned char crc = msg.body[0];
    int i;
    for(i = 1; i < length; i++)
    {
        crc ^= msg.body[i];
        //CRC8(crc, body[i]);
    }
    msg.checkbyte = crc;
    return msg;
}

struct message depack_msg(char *buf, int buf_len)
{
    struct message msg;
    memcpy(&msg, buf, buf_len);
    return msg;
}

void print_msg_info(struct message *msg)
{
    printf("szteam = %s\n", msg->szteam);
    printf("type = %d\n", msg->type);
    printf("pack_no = %d\n", msg->pack_no);
    printf("length = %d\n", msg->length);
    printf("checkbyte = %x\n", msg->checkbyte);
    printf("msg size = %d\n", (int)sizeof(*msg));

    printf("data[0] = %c\n", msg->body[0]);
    printf("data[1023] = %c\n", msg->body[1023]);
}

void do_print(int thread_no, char *msg)
{
    printf("[Thread: %d] %s\n", thread_no, msg);
}

int SYN_solution(int sockfd, struct message *msg, struct sockaddr pservaddr)
{
    return 0;
}
int ACK_solution(int sockfd, struct message *msg, struct sockaddr pservaddr)
{
    return 0;
}
int FIN_solution(int sockfd, struct message *msg, struct sockaddr pservaddr)
{
    return 0;
}
int DATA_solution(int sockfd, struct message *msg, struct sockaddr pservaddr)
{
    return 0;
}

void init_protocal_buf()
{
    // 初始化协议栈
    int i;
    for(i = 0; i < TYPE_NUM; i++)
    {
        protocal_buf[i] = NULL;
    }
    protocal_buf[SYN] = SYN_solution;
    protocal_buf[ACK] = ACK_solution;
    protocal_buf[FIN] = FIN_solution;
    protocal_buf[DATA] = DATA_solution;
}

size_t get_filesize(int fd)
{
    // 获取文件大小
    struct stat st;
    fstat(fd,&st);
    return st.st_size;
}

// 打印进度百分比
void print_process(int percent,int barlen){
    int i;
    putchar('[');
    for(i=1;i<=barlen;++i)
        putchar(i*100<=percent*barlen?'>':' ');
    putchar(']');
    printf("%3d%%",percent);
    for(i=0;i!=barlen+6;++i)
        putchar('\b');
    
    fflush(stdout);
}
