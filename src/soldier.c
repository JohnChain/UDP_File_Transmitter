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

    printf("[thread: %d] here in subthread\n", thread_no);
    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr pcliaddr;
    socklen_t cliaddr_len = sizeof(pcliaddr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bzero(&(servaddr.sin_zero),8);

    /** set socket timeout time 1s **/
    struct timeval tv;
    tv.tv_sec  = SOCK_TIMEOUT_SEC;
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
    FILE* read_fd;

    /** block here to receive pack from client **/
    int recv_len = recvfrom(sockfd, syn_buf, sizeof(struct message), 0, &pcliaddr, &cliaddr_len);
    if(recv_len == -1)
    {
        printf("[thread: %d]first recv timeout or sock error, errno = %d\n", thread_no, errno);
        return NULL;
    } else if(recv_len != sizeof(struct message))
    {
        printf("[thread: %d]got one wrong length pack\n", thread_no);
        return NULL;
    }

    syn_msg = depack_msg(syn_buf, recv_len);
    struct synack syn_pack;
    memcpy(&syn_pack, syn_msg.body, syn_msg.length);
    printf("[thread: %d]receive:downloadable = %d, thread_num = %d, port[0] = %d, port[1] = %d, port[2] = %d, filename = %s, filesize = %ld, breakpoint = %d, endpoint = %d, blocksize = %d, blocknum = %d, downloaded_block = %d\n", thread_no, syn_pack.downloadable, syn_pack.thread_num,syn_pack.port[0], syn_pack.port[1], syn_pack.port[2], syn_pack.file_info.filename, syn_pack.file_info.filesize, syn_pack.file_info.breakpoint, syn_pack.file_info.end_point, syn_pack.file_info.blocksize, syn_pack.file_info.block_num, syn_pack.file_info.downloaded_block);

    read_fd= fopen(syn_pack.file_info.filename, "rb");
    if(read_fd == NULL)
    {
        printf("[thread: %d]open file fialed\n", thread_no);
        return NULL;
    }
    printf("[thread: %d]open file %s successfully, fileno = %d\n",thread_no, syn_pack.file_info.filename, fileno(read_fd));

    /** set the seek number **/
    int downloadable = 1;
    //int thread_num; //redefined
    char *filename = syn_pack.file_info.filename;
    int filesize = (int)get_filesize(fileno(read_fd)); /* size of file */
    int breakpoint;
    int end_point; /* the last pack_no for each thread*/
    int blocksize = BODYLEN; /* size of each block */
    int block_num; /* parts file divided */
    int last_pack; /* the pack_no this subthread received last time **/
    long seek_num; /* position of file to read or write from */

    // EXAMPLE:
    // threads :    1, 2, 3
    // block_num:   20
    // end_point:   6,    12   ,    20
    // start_point: 1,    7    ,    13
    // last_pack:   0,    6    ,    12
    // seek_num:    0, 1024 * 6, 1024 * 12

    // count block_num
    if(filesize % BODYLEN == 0)
        block_num = filesize / BODYLEN;
    else block_num = filesize / BODYLEN + 1;


    if(thread_no != thread_num)
        /** not the last thread **/
        end_point = thread_no * (block_num / thread_num);
    else
        /** the last thread **/
        end_point = block_num;

    if(syn_pack.thread_num == 0)
    {
        last_pack = (thread_no - 1) * (block_num / thread_num); 
        seek_num = last_pack * BODYLEN;
    } else {
        last_pack = syn_pack.file_info.breakpoint; 
        seek_num = syn_pack.file_info.breakpoint * BODYLEN;
        blocksize = syn_pack.file_info.blocksize;
    }

    if(fseek(read_fd, seek_num, SEEK_SET) == -1)
    {
        printf("[thread: %d] Seek error!\n", thread_no);
        return NULL;
    }

    /** send the synack pack to client **/
    struct synack synack_pack;

    synack_pack.downloadable = 1;
    synack_pack.thread_num = thread_num;
    strcpy(synack_pack.file_info.filename, filename);
    synack_pack.file_info.filesize = filesize;
    synack_pack.file_info.breakpoint = last_pack;
    synack_pack.file_info.end_point = end_point;
    synack_pack.file_info.blocksize = blocksize;
    synack_pack.file_info.block_num= block_num;
    synack_pack.file_info.downloaded_block = downloaded_block;

    printf("[thread: %d]send:downloadable = %d, thread_num = %d, port[0] = %d, port[1] = %d, port[2] = %d, filename = %s, filesize = %d, breakpoint = %d, endpoint = %d, blocksize = %d, blocknum = %d, downloaded_block = %d\n", thread_no, synack_pack.downloadable, synack_pack.thread_num,synack_pack.port[0], synack_pack.port[1], synack_pack.port[2], synack_pack.file_info.filename, synack_pack.file_info.filesize, synack_pack.file_info.breakpoint, synack_pack.file_info.end_point, synack_pack.file_info.blocksize, synack_pack.file_info.block_num, synack_pack.file_info.downloaded_block);

    struct message synack_msg= pack_msg(SYN_ACK, 0, sizeof(struct synack), &synack_pack);
    sendto(sockfd, &synack_msg, sizeof(struct message), 0, &pcliaddr, cliaddr_len);
    if(last_pack >= end_point)
    {
        printf("[thread: %d]partly finished\n", thread_no);
        fclose(read_fd);
        close(sockfd);
        return NULL; 
    }

    /** read the file data from breakpoint, and send data to client **/
    int bytes_read = 1;
    int bytes_write = -1;
    int bytes_sent = -1;
    char part_file[LEN_FILENAME];
    sprintf(part_file, "part_file_%d", thread_no);
    FILE* write_fd = fopen(part_file, "wb+");
    if(write_fd  == NULL)
    {
        printf("[thread: %d]open write file error\n", thread_no);
        fclose(read_fd);
        close(sockfd);
        return NULL;
    }

    char temp_buf[BODYLEN];
    int count = 0; /** count the packages this thread has send **/
    int pack_no = last_pack; /* start from last_pack + 1 */
    printf("[thread: %d]starting from pack_no = %d\n", thread_no, pack_no);
    struct message data_msg;
    while(bytes_read > 0 && pack_no < end_point) 
    { 
        memset(temp_buf, 0, sizeof(char) * BODYLEN);
        bytes_read = fread(temp_buf, sizeof(char), BODYLEN, read_fd);

        data_msg = pack_msg(DATA, ++pack_no, bytes_read, temp_buf);

        //print_msg_info(&data_msg);

        bytes_write = fwrite(data_msg.body, sizeof(char), bytes_read, write_fd);
        fflush(write_fd);

        char output_str[100];
        sprintf(output_str, "============================write byte num = %d, errno = %d\n", bytes_write, errno);
        do_print(thread_no, output_str);

        bytes_sent = sendto(sockfd, &data_msg, sizeof(data_msg), 0, &pcliaddr, cliaddr_len);
        sprintf(output_str, "================ pack_no: %d ============send byte num = %d, errno = %d\n", pack_no, bytes_sent, errno);
        do_print(thread_no, output_str);

        if(bytes_sent < 0)
        {
            printf("[thread: %d]send error, transmit canciled\n", thread_no);
            break;
        }
        count ++;

        //unsigned char crc = data_msg.body[0];
        //int i;
        //for(i = 1; i < data_msg.length; i++)
        //{
            //crc ^= data_msg.body[i];
            ////CRC8(crc, temp_msg->body[i]);
        //}
        ////printf("local crc = %x\n", crc);

        /** wait for ack pack **/
        char buf[MAXLEN];
        int resend_times = 1;
        int client_end = NO;
        while(resend_times <= RESENDTIME && client_end == NO)
        {
            printf("[thread: %d]waitting for data ack\n", thread_no);
            recv_len = recvfrom(sockfd, buf, sizeof(struct message), 0, &pcliaddr, &cliaddr_len);
            printf("[thread: %d] receive byte = %d\n", thread_no, recv_len);
            if(recv_len == sizeof(struct message))
            {
                printf("[thread: %d]here in first condition\n", thread_no);
                struct message ack_msg = depack_msg(buf, recv_len);
                if(ack_msg.type == FIN)
                {
                    printf("[thread: %d] partly finished!\n", thread_no);
                    client_end = YES;
                    break;
                }else if(ack_msg.type == ACK)
                {
                    //TODO: check the ack body, if pack error, resend or not ?
                    printf("[thread: %d] got one dack ack pack, pack_no = %d, errono = %d\n", thread_no, ack_msg.pack_no, errno);
                    if(ack_msg.pack_no == pack_no)
                        break;
                }else if(ack_msg.type == SYN){
                    printf("[thred: %d] got one SYN pack, resend SYNACK to client", thread_no);
                    sendto(sockfd, &synack_msg, sizeof(struct message), 0, &pcliaddr, cliaddr_len);
                    resend_times --;
                }
            //}else if(recv_len == EAGAIN || recv_len == EWOULDBLOCK ) { 
                //printf("here in second condition\n");
                ///** time out, resend pack **/
                //bytes_sent = sendto(sockfd, &data_msg, sizeof(data_msg), 0, &pcliaddr, cliaddr_len);
                //printf("[thread: %d]>>>>>>>>>>>>>>>>>>>>>>>..recsent pack: %d\n", thread_no, pack_no);
                ////if(resend_times > 3)
                    ////break;
            }else{
                printf("[thread: %d]here in other cond\n", thread_no);
                printf("[thread: %d]EAGAIN = %d, EWOULDBLOCK = %d\n", thread_no, EAGAIN, EWOULDBLOCK);
                bytes_sent = sendto(sockfd, &data_msg, sizeof(data_msg), 0, &pcliaddr, cliaddr_len);
                printf("[thread: %d]>>>>>>>>>>>>>>>>>>>>>>>..recsent pack: %d, errno = %d\n", thread_no, pack_no, errno);
            }
            resend_times++;
        }
        if(resend_times > RESENDTIME || client_end == YES)
            break;
    }
    printf("[thread: %d] send done!\n", thread_no);
    struct message fin_msg = pack_msg(FIN, 0, 0, "");
    sendto(sockfd, &fin_msg, sizeof(fin_msg), 0, &pcliaddr, cliaddr_len);
    printf("[thread: %d]count = %d\n",thread_no, count);
    fclose(read_fd);
    fclose(write_fd);

    close(sockfd);
    return NULL;
}

