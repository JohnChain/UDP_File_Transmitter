#include"udpserver.h"

/***
 ** the main function of subthread
 ** param task: all values to be used in subthread, type of struct subthread_task
 ***/
void * mission(void * task)
{    
    int thread_no = ((struct subthread_task *)task)->thread_no;
    int port = ((struct subthread_task *)task)->port;
    int thread_num = ((struct subthread_task *)task)->thread_num;
    struct local_file file_info = ((struct subthread_task *)task)->file_info;

    printf("[thread: %d] here in subthread\n", thread_no);
    int sockfd;
    struct sockaddr_in servaddr;
    //struct sockaddr pcliaddr;
    struct sockaddr_in cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bzero(&(servaddr.sin_zero),8);

    /** set socket timeout time 1s **/
    struct timeval tv;
    tv.tv_sec  = SOCK_TIMEOUT_SEC + 10;
    tv.tv_usec = SOCK_TIMEOUT_USEC;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        printf("[thread: %d]socket option  SO_RCVTIMEO not support\n", thread_no);
        return 0;
    }

    /** bind address and port to socket **/
    if (bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind error");
        return NULL;
    }

    printf("[thread: %d]port bind finished, waiting for you~\n", thread_no);
    char syn_buf[MAXLEN];
    struct message syn_msg;

    /** block here to receive pack from client **/
    int timeout_count = 0;
    int recv_len = -1;
    while(timeout_count < RESENDTIME){
        recv_len = recvfrom(sockfd, syn_buf, sizeof(struct message), 0, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if(recv_len == -1) {
            printf("[thread: %d]first recv timeout or sock error, errno = %d\n", thread_no, errno);
            timeout_count ++;
        } else {
            printf("[thread: %d] sizeof msg_synack = %d\n", sizeof(struct msg_synack));
            printf("[thread: %d] got SYN done ,bytes received: %d\n", thread_no, recv_len);
            break;
        }
    }
    if(timeout_count == RESENDTIME) return NULL;

    /* make new socket to the ACK-Receving Port on client */ 
    struct sockaddr_in new_addr;
    bzero(&new_addr, sizeof(new_addr));
    new_addr.sin_family = AF_INET;
    new_addr.sin_port = htons(8889);
    new_addr.sin_addr = cliaddr.sin_addr;

    int sockfd2 = socket(AF_INET, SOCK_DGRAM, 0); 
    //printf("sending to new server\n");
    //bytes_send = sendto(sockfd2, "server hello", 20, 0, (struct sockaddr *)&new_addr, sizeof(new_addr));
    //printf("send bytes = %d\n", bytes_send);


    syn_msg = depack_msg(syn_buf, recv_len);
    struct synack syn_pack;
    memcpy(&syn_pack, syn_msg.body, syn_msg.length);
    //print_synack_info(thread_no, syn_pack);
    printf("[thread: %d]receive:downloadable = %d, thread_num = %d, \
            port[0] = %d, filename = %s, \
            filesize = %ld, breakpoint = %d, endpoint = %d, blocksize = %d, \
            blocknum = %d, downloaded_block = %d\n", thread_no, syn_pack.downloadable, syn_pack.thread_num,\
            syn_pack.port[0], syn_pack.file_info.filename, \
            syn_pack.file_info.filesize, syn_pack.file_info.breakpoint, syn_pack.file_info.end_point, \
            syn_pack.file_info.blocksize, syn_pack.file_info.block_num, syn_pack.file_info.downloaded_block);



    /** send the synack pack to client **/
    struct synack synack_pack;
    synack_pack.downloadable = 1;
    synack_pack.thread_num = thread_num;
    synack_pack.port[0] = port;
    synack_pack.file_info = file_info;

    printf("[thread: %d]receive:downloadable = %d, thread_num = %d, \
            port[0] = %d, filename = %s, \
            filesize = %ld, breakpoint = %d, endpoint = %d, blocksize = %d, \
            blocknum = %d, downloaded_block = %d\n", thread_no, syn_pack.downloadable, syn_pack.thread_num,\
            syn_pack.port[0], syn_pack.file_info.filename, \
            syn_pack.file_info.filesize, syn_pack.file_info.breakpoint, syn_pack.file_info.end_point, \
            syn_pack.file_info.blocksize, syn_pack.file_info.block_num, syn_pack.file_info.downloaded_block);

    //print_synack_info(thread_no, synack_pack);
    //struct message synack_msg= pack_msg(SYN_ACK, 0, sizeof(struct synack), &synack_pack);
    struct msg_synack synack_msg = pack_msg_synack(SYN_ACK, 0, sizeof(struct synack), synack_pack);
    sendto(sockfd, &synack_msg, sizeof(struct msg_synack), 0, (struct sockaddr *)&cliaddr, cliaddr_len);

    /** reset socket timeout **/
    tv.tv_sec  = SOCK_TIMEOUT_SEC;
    tv.tv_usec = SOCK_TIMEOUT_USEC;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        printf("[thread: %d]socket option  SO_RCVTIMEO not support\n", thread_no);
        return 0;
    }

    /** read the file data from breakpoint, and send data to client **/
    int bytes_sent = -1;
    int count = 0; /** count the packages this thread has send **/
    struct message data_msg;

    int client_end = NO;
    int timeout_times = 0;
    struct timeval start_time, end_time;
    while(client_end == NO && timeout_times < RESENDTIME)
    { 
        /** wait for data pack **/
        char buf[MAXLEN];

        //printf("[thread: %d]waitting for data \n", thread_no);
        //gettimeofday(&start_time, NULL);
        recv_len = recvfrom(sockfd, buf, sizeof(struct message), 0, (struct sockaddr *)&cliaddr, &cliaddr_len);
        //gettimeofday(&end_time, NULL);
        //pthread_mutex_lock(&lock_cost_recv);
        //cost_recv += (float)((end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec));
        //pthread_mutex_unlock(&lock_cost_recv);
        
        
        //printf("[thread: %d] receive byte = %d\n", thread_no, recv_len);
        if(recv_len == sizeof(struct message))
        {
            //gettimeofday(&start_time, NULL);
            struct message data_msg = depack_msg(buf, recv_len);
            if(data_msg.type == DATA) {
                //printf("[thread: %d]here in first condition, pack_no = %d\n", thread_no, data_msg.pack_no);

                /* 发送数据确认 */
                struct synack synack_pack;
                struct msg_synack ack_msg = pack_msg_synack(ACK, data_msg.pack_no, sizeof(struct synack), synack_pack);
                //bytes_sent = sendto(sockfd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
                bytes_sent = sendto(sockfd2, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&new_addr, sizeof(new_addr));

                //printf("[thread: %d] send ===== ACK pack_no: %d ======= byte num = %d, errno = %d\n", thread_no, data_msg.pack_no, bytes_sent, errno);

                if(bytes_sent < 0) {
                    printf("[thread: %d]send error, transmit canciled\n", thread_no);
                    break;
                }

                /* 将数据加入队列 */
                Append_Queue_Item(data_queue, file_info.filename, data_msg.pack_no, data_msg.body, data_msg.length);
                count ++;
                timeout_times = 0;
            }
        }else if(recv_len == sizeof(struct msg_synack)){
            struct message data_msg = depack_msg(buf, recv_len);
            if(data_msg.type == FIN) {
                printf("[thread: %d] get FIN, partly finished!\n", thread_no);
                client_end = YES;
                break;
            }else if(data_msg.type == SYN){
                printf("[thred: %d] got one SYN pack, resend SYNACK to client", thread_no);
                sendto(sockfd, &synack_msg, sizeof(struct msg_synack), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
            }

        }else{
            printf("[thread: %d]here in receive timeout \n", thread_no);
            printf("[thread: %d]EAGAIN = %d, EWOULDBLOCK = %d\n", thread_no, EAGAIN, EWOULDBLOCK);
            //bytes_sent = sendto(sockfd, &data_msg, sizeof(data_msg), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
            //printf("[thread: %d]>>>>>>>>>>>>>>>>>>>>>>>..recsent pack: %d, errno = %d\n", thread_no, data_msg.pack_no, errno);
            timeout_times ++;
        }
        
        //printf("[thread: %d] Total cost: cost_recv = %f, cost_write = %f, cost_pack = %f, cost_send = %f\n", thread_no, cost_recv / 1000000, cost_send / 1000000, cost_pack / 1000000, cost_send / 1000000);

    }

    printf("[thread: %d] send done!\n", thread_no);
    struct synack fin_pack ;
    struct msg_synack fin_msg = pack_msg_synack(FIN, 0, sizeof(struct synack), fin_pack);
    //sendto(sockfd, &fin_msg, sizeof(fin_msg), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
    sendto(sockfd2, &fin_msg, sizeof(fin_msg), 0, (struct sockaddr *)&new_addr, sizeof(new_addr));
    printf("[thread: %d]count = %d\n",thread_no, count);
    close(sockfd);

    return NULL;
}

