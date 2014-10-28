#include"udpserver.h"

/***
 ** main thread function, assign ports and tasks to subthreads
 ** param sockfd : socket fileno 
 ** param pcliaddr: used for remember the connected client info
 ** param client: sizeof pcliaddr
 ***/      
void commander(int sockfd, struct sockaddr* pcliaddr, socklen_t client)
{
    char package[MAXLEN];
    int recv_len;
    int base_port = 30000;
    int port_range = 5000;
    int shift = 1;
    pthread_t ptid[port_range];

    data_queue = Initialize_Queue();

    /* 开启写操作线程 */
    writer_live = 1;
    pthread_t mag_tid;
    pthread_create(&mag_tid, NULL, writer, NULL);
            
    for(;;) {
        recv_len = recvfrom(sockfd, package, sizeof(struct message), 0, pcliaddr, &client);
        printf("recv_len = %d\n", recv_len);
        if(recv_len == sizeof(struct msg_synack)) {
            struct message syn_msg = depack_msg(package, recv_len);
            struct synack syn_pack; //= syn_msg.synack;
            memcpy(&syn_pack, syn_msg.body, sizeof(struct synack));
            printf("receive:downloadable = %d, thread_num = %d, port[0] = %d, port[1] = %d, port[2] = %d, filename = %s, filesize = %d, breakpoint = %d, endpoint = %d, blocksize = %d, blocknum = %d, downloaded_block = %d\n",syn_pack.downloadable, syn_pack.thread_num,syn_pack.port[0], syn_pack.port[1], syn_pack.port[2], syn_pack.file_info.filename, syn_pack.file_info.filesize, syn_pack.file_info.breakpoint, syn_pack.file_info.end_point, syn_pack.file_info.blocksize, syn_pack.file_info.block_num, syn_pack.file_info.downloaded_block);

            struct synack synack_pack;
            synack_pack = syn_pack;
            
            downloaded_block = syn_pack.file_info.downloaded_block;

            /** shift will round from 1 to port_range **/
            int new_port;
            shift %= port_range;
            new_port = base_port + shift;
            shift += synack_pack.thread_num;
            int i;
            for(i = 0; i < synack_pack.thread_num; i++) {
                synack_pack.port[i] = new_port + i;
            }

            printf("send: thread_num = %d, downloaded_block = %d, block_num = %d\n", synack_pack.thread_num, synack_pack.file_info.downloaded_block, synack_pack.file_info.block_num);
            printf("send: port: %d,%d, %d\n", synack_pack.port[0], synack_pack.port[1], synack_pack.port[2]);

            struct msg_synack synack_msg = pack_msg_synack(SYN_ACK, 0, sizeof(struct synack), synack_pack); 

            /** TODO: 开启多线程发送**/
            cost_recv = 0;
            cost_write = 0;
            cost_pack = 0;
            cost_send = 0;

            //lock_cost_recv  = PTHREAD_MUTEX_INITIALIZER;
            //lock_cost_write = PTHREAD_MUTEX_INITIALIZER;
            //lock_cost_pack  = PTHREAD_MUTEX_INITIALIZER;
            //lock_cost_send  = PTHREAD_MUTEX_INITIALIZER;
            
            pthread_mutex_init(&lock_cost_recv, NULL);
            pthread_mutex_init(&lock_cost_send, NULL);
            pthread_mutex_init(&lock_cost_pack, NULL);
            pthread_mutex_init(&lock_cost_write, NULL);

            struct subthread_task *task_list = (struct subthread_task *)malloc(sizeof(struct subthread_task) * synack_pack.thread_num);
            for(i = 0; i < synack_pack.thread_num; i++) {
                task_list[i].port = synack_pack.port[i];
                task_list[i].thread_no = i + 1;
                task_list[i].thread_num = synack_pack.thread_num;
                task_list[i].file_info = synack_pack.file_info;
                pthread_create(&ptid[i], NULL, mission, &task_list[i]);
            }

            printf("send:downloadable = %d, thread_num = %d, port[0] = %d, port[1] = %d, port[2] = %d, filename = %s, filesize = %d, breakpoint = %d, endpoint = %d, blocksize = %d, blocknum = %d, downloaded_block = %d\n",synack_pack.downloadable, synack_pack.thread_num,synack_pack.port[0], synack_pack.port[1], synack_pack.port[2], synack_pack.file_info.filename, synack_pack.file_info.filesize, synack_pack.file_info.breakpoint, synack_pack.file_info.end_point, synack_pack.file_info.blocksize, synack_pack.file_info.block_num, synack_pack.file_info.downloaded_block);

            char *tempbuf;
            tempbuf = (char *)malloc(sizeof(struct msg_synack));
            memcpy(tempbuf, &synack_msg, sizeof(synack_msg));
            int bytes_send = sendto(sockfd, tempbuf, sizeof(struct msg_synack), 0, pcliaddr, client);
            free(tempbuf);
            printf("///////////////send byte %d\n", bytes_send);
            
            //for(i = 0; i < synack_pack.thread_num; i++) {
                //pthread_join(ptid[i], NULL);
            //}

            //Free_Queue(data_queue);

            printf("waiting for next client\n");

        }else {
            printf("server got illegal message\n");
        }
    }
}

void process_end(int signo){
    printf("Here get 'END THE PROCESS' signal, ending subthreads...\n");
    writer_live = 0;
    Free_Queue(data_queue);
    exit(1);
}

int main(int argc, char* argv[])
{
    init_protocal_buf();

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    /** bind address and port to socket **/
    if (bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind error");
        exit(1);
    }
    
    signal(SIGINT, process_end);
    signal(SIGKILL, process_end);

    commander(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
    return 0;
}

