#include"udpclient.h"

void usage();
void process_end(int signo);
struct synack get_new_port(struct synack file_info, int sockfd, struct sockaddr* pservaddr, socklen_t servlen);
void *receive_file(void *task);
void close_cocket(void *sock_hd);
void close_file(void *fd);

int main(int argc, char* argv[])
{
    if(argc != 4)
    {
        usage();
        exit(1);
    }
    
    init_protocal_buf();
    pthread_mutex_init(&down_lock, NULL);

    /** init download file and set the task_flag **/
    if(access(argv[3], 0) == -1)
    {
        FILE *temp_handler = fopen(argv[3], "a+");
        fclose(temp_handler);
        task_flag = NEW;

    }else task_flag = OLD;

    /** init server address **/
    int sockfd;
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        printf("[%s] is not avalid IP address!\n", argv[1]);
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct synack syn_pack;
    strcpy(syn_pack.file_info.filename, argv[3]);
    printf("filename = %s\n", syn_pack.file_info.filename);

    if(task_flag == NEW)
    {
        syn_pack.thread_num = 0;
        syn_pack.file_info.block_num = 0;
        syn_pack.file_info.downloaded_block = 0;
        syn_pack.file_info.blocksize = 0;

        downloaded_block = 0;
    } else {
        /** get last time transmition state **/
        config_t *conf_hd = my_confRead();
        syn_pack.thread_num = my_confGet(conf_hd, "thread_num");
        syn_pack.file_info.block_num = my_confGet(conf_hd, "block_num");
        syn_pack.file_info.blocksize = my_confGet(conf_hd, "blocksize");
        syn_pack.file_info.filesize= my_confGet(conf_hd, "filesize");
        confDestory(conf_hd);
        printf("block_num = %d, thread_num = %d, blocksize = %d, filesize = %ld\n", syn_pack.file_info.block_num, syn_pack.thread_num, syn_pack.file_info.blocksize, syn_pack.file_info.filesize);

        /** count local global downloaded_block **/
        downloaded_block = 0;
        int i; 
        int downloaded_size;
        for(i = 1; i <= syn_pack.thread_num; i++)
        {
            char part_target[LEN_FILENAME];
            sprintf(part_target, ".part_target_%d", i);

            int temp_size;
            int fd = open(part_target, O_RDONLY);
            temp_size = get_filesize(fd);
            close(fd);

            downloaded_block += (temp_size / syn_pack.file_info.blocksize);
            downloaded_size += temp_size;
        }
        syn_pack.file_info.downloaded_block = downloaded_block;
        if(downloaded_size == syn_pack.file_info.filesize)
        {
            printf("download already finished\n");
            return 0;
        }
    }
    printf("figure out downloaded_block = %d\n", downloaded_block);

    struct synack synack_pack = get_new_port(syn_pack, sockfd, (struct sockaddr*)(&servaddr), sizeof(servaddr));
    printf("receive: thread_num = %d, downloaded_block = %d, block_num = %d\n", synack_pack.thread_num, synack_pack.file_info.downloaded_block, synack_pack.file_info.block_num);

    if(synack_pack.downloadable == 0)
    {
        printf("sorry, now such file named %s\n", argv[3]);
        return 0;
    }

    /** transmition state-synchronization finished **/
    thread_num = synack_pack.thread_num;
    block_num = synack_pack.file_info.block_num;
    blocksize = synack_pack.file_info.blocksize;
    filesize = synack_pack.file_info.filesize;
    downloaded_block = synack_pack.file_info.downloaded_block;

    /** init config file, can be expended later for any request **/
    init_conf();
    config_t * conf_hd = my_confRead();
    my_confSet(conf_hd, "thread_num", thread_num);
    my_confSet(conf_hd, "block_num", block_num);
    my_confSet(conf_hd, "blocksize", blocksize);
    my_confSet(conf_hd, "filesize", filesize);

    confWrite(conf_hd);
    confDestory(conf_hd);

    struct subthread_task *task_list = (struct subthread_task *)malloc(sizeof(struct subthread_task) * thread_num);
    int i;
    for(i = 0; i < thread_num; i++)
    {
        task_list[i].hostname = argv[1];
        task_list[i].port = synack_pack.port[i];

        task_list[i].thread_no = i + 1;
        task_list[i].thread_num = thread_num;

        strcpy(task_list[i].file_info.filename, argv[3]);
        task_list[i].file_info.filesize= filesize;

        task_list[i].file_info.block_num = block_num;
        task_list[i].file_info.blocksize= blocksize;

        pthread_create(&ptid[i], NULL, receive_file, &task_list[i]);
    }

    signal(SIGINT, process_end);
    signal(SIGKILL, process_end);

    do{
        sleep(0.1);
        int percent = (downloaded_block * 100) / block_num;
        print_process(percent, 100); 
    }while(downloaded_block < block_num);
    print_process(100, 100); 
    printf("\ndownload finished, exiting...[please wait and not interrupt or power off]\n");
    
    //printf("part_target_s flushed\n"); 
    //fflush(tfd);

    /** merge all part_targets to target file **/
    FILE *target_file = fopen(argv[3], "wb+");
    FILE *part_target;
    char part_target_name[LEN_FILENAME];
    for(i = 1; i <= thread_num; i++)
    {
        sprintf(part_target_name, ".part_target_%d", i);
        part_target = fopen(part_target_name, "rb");
        if(part_target == NULL)
        {
            printf("open part target file %d failed\n", i);
            continue;
        }
        printf("merging part file from thread_%d...\n", i);
        char temp_buf[BODYLEN];
        int byte_read = 1;
        int byte_write = 0;
        while(byte_read > 0)
        {
            byte_read = fread(temp_buf, sizeof(char), BODYLEN, part_target);
            if(byte_read == -1)
            {
                printf("error to merge all target park\n");
                break;
            }
            byte_write += fwrite(temp_buf, sizeof(char), byte_read, target_file);
        }
        /** count each part file write to target **/
        printf("from part_target_%d write to target %d \n", i, byte_write);
        fclose(part_target);
        //// TODO: remove the cache file
        remove(part_target_name);
    }
    fflush(target_file);
    fclose(target_file);

    int tb_fno = open(".part_target_1", O_RDONLY);
    int new_target_size = get_filesize(tb_fno);
    printf("second open filesize is:%d\n", new_target_size);
    lseek(tb_fno, 22310911, SEEK_SET);
    char tt_buf[2050];
    int rd_len = read(tb_fno, tt_buf, 2050);
    //int dirt_part = open("dirt_part1", O_WRONLY|O_CREAT, 0666);
    //write(dirt_part, tt_buf, rd_len);
    close(tb_fno);
    //close(dirt_part);

    pthread_mutex_destroy(&down_lock);
    free(task_list);
    return 0;
}

/***
 ** Ctrl+C signal dealing function
 ** param signal: signal number
 ***/
void process_end(int signo)
{
    printf("Here get 'END THE PROCESS' signal, ending subthreads...\n");
    int i;
    for(i = 0; i < thread_num; i++)
        pthread_cancel(ptid[i]);
    exit(1);
}

/***
 ** print the usage
 ***/
void usage()
{
    printf("USAGE: client hostname port filename\n");
}


/***
 ** subthread main program
 ***/
void *receive_file(void *task)
{
    /** set subthread unable cancel **/
    //int setcancel_rst;
    //int oldstate = 0;
    //int oldtype = 0;
    //setcancel_rst = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    //setcancel_rst = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

    struct subthread_task local_task = *((struct subthread_task *)task);
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(local_task.port);
    if(inet_pton(AF_INET, local_task.hostname, &servaddr.sin_addr) <= 0)
    {
        printf("[%s] is not avalid IP address!\n", local_task.hostname);
        return NULL;
    }
    if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
    {
        perror("connect error");
        return NULL;
    }
    
    char part_target[LEN_FILENAME];
    sprintf(part_target, ".part_target_%d", local_task.thread_no);

    struct message syn_msg;
    struct synack syn_pack;
    int downloaded_size;
    strcpy(syn_pack.file_info.filename, local_task.file_info.filename);
    syn_pack.file_info.end_point = 0; /* wait for server filling it */
    syn_pack.file_info.blocksize = local_task.file_info.blocksize; 
    if(task_flag == OLD)
    {
        printf(" >>>>>> this is a old task\n");
        syn_pack.thread_num = local_task.thread_num;
        /** figure out the breakpoint **/
        int fd = open(part_target, O_RDONLY);
        if(fd == -1)
        {
            FILE *fp = fopen(part_target, "w+");
            fclose(fp);
            downloaded_size = 0;
        }else
        {
            downloaded_size = get_filesize(fd);
            close(fd);
        }

        syn_pack.file_info.breakpoint = (downloaded_size / local_task.file_info.blocksize) + \
                                        (local_task.thread_no - 1) * (local_task.file_info.block_num / local_task.thread_num);
        printf("(%d / %d) + (%d - 1) * (%d / %d)\n",downloaded_size, local_task.file_info.blocksize,local_task.thread_no, local_task.file_info.block_num, local_task.thread_num);
    }else
    {
        printf("[thread: %d] >>>>>> this is a new task \n", local_task.thread_no);
        syn_pack.thread_num = 0; /* setted to zero for making server knowing this is a new task */
        syn_pack.file_info.breakpoint = 0;
        downloaded_size = 0;
    }
    syn_msg = pack_msg(SYN, 0, sizeof(syn_pack), &syn_pack);
    printf("[thread: %d]>>>>>>>>local_info: thread_num = %d, end_point = %d, breakpoint = %d\n", local_task.thread_no, syn_pack.thread_num, syn_pack.file_info.end_point, syn_pack.file_info.breakpoint);
    send(sockfd, &syn_msg, sizeof(struct message), 0);


    /** find out the start point for old task **/
    FILE* writefd; 
    if(task_flag == OLD)
    {
        char part_target_old[LEN_FILENAME];
        sprintf(part_target_old, ".part_target_%d_old", local_task.thread_no);
        rename(part_target, part_target_old);

        FILE* writefd_old = fopen(part_target_old, "rb+");
        FILE* writefd = fopen(part_target, "wb+");
        //setvbuf(writefd, buf, _IOFBF, 8192);
        if(writefd == NULL)
        {
            close(sockfd);
            perror("open dest file error");
            return NULL;
        }

        int downloaded = downloaded_size / local_task.file_info.blocksize;
        int no = 0;
        for(no = 0; no < downloaded; no++)
        {
            char temp_buf[BODYLEN];
            int read_len = fread(temp_buf, sizeof(char), local_task.file_info.blocksize, writefd_old);
            fwrite(temp_buf, sizeof(char), read_len, writefd);
        }
        fclose(writefd_old);
        //if(local_task.thread_no == 1)
            //tfd = writefd;
        remove(part_target_old);
        //return NULL;
        /** watch out here, not closing it will make you a great plroblem **/
        fclose(writefd); 
    }
    writefd = fopen(part_target, "ab+");

    int tb_fno = open(part_target, O_RDONLY);
    fpos_t pst;
    fgetpos(writefd, &pst);
    printf("[thread: %d]second append open start at :%d\n", local_task.thread_no, pst);
    int new_target_size = get_filesize(tb_fno);
    printf("[thread: %d]second open filesize is:%d\n", local_task.thread_no, new_target_size);
    close(tb_fno);


    /** set socket timeout time **/
    struct timeval tv;
    tv.tv_sec = SOCK_TIMEOUT_SEC;
    tv.tv_usec = SOCK_TIMEOUT_USEC;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        printf("socket option  SO_RCVTIMEO not support\n");
        close(sockfd);
        return NULL;
    }

    int count = 0;
    int recv_len = 1;
    char temp_buf[MAXLEN];

    int bytes_write = 0;
    int bytes_send = 0;

    int last_pack = 0;
    int end_point = 0;

    //setcancel_rst = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    //setcancel_rst = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    //pthread_cleanup_push(close_file, writefd);
    //pthread_cleanup_push(close_cocket, &sockfd);

    while(recv_len > 0) 
    {
        /*receive data from sever*/
        recv_len = recv(sockfd, temp_buf, sizeof(char) * MAXLEN, 0);
        if(recv_len == -1)
        {
            //perror("time out or read error");
            printf("[thread: %d]timeout or read error\n", local_task.thread_no);
            break;
        }

        struct message temp_msg = depack_msg(temp_buf, recv_len);

        if(temp_msg.type == SYN_ACK)
        {
            struct synack synack_pack;
            memcpy(&synack_pack, temp_msg.body, temp_msg.length);
            end_point = synack_pack.file_info.end_point;
            last_pack = synack_pack.file_info.breakpoint;
            blocksize = synack_pack.file_info.blocksize;
            if(last_pack >= end_point)
            {
                //printf("[thread: %d] partly finished! \n", local_task.thread_no);
                break;
            }

            printf("[thread: %d] here got one synack, end point = %d, last_pack = %d\n", local_task.thread_no, end_point, last_pack);
            continue;
        }else if(temp_msg.type == FIN)
        {
            //printf("[thread: %d] partly finished!\n", local_task.thread_no);
            printf("[thread: %d] return from FIN\n", local_task.thread_no);
            break;
        }

        //print_msg_info(&temp_msg);

        /** check the pack_no legality **/
        if(temp_msg.pack_no > (last_pack + 1)) {
            printf("\n[thread: %d]<<<<<<<<<<<<<<<<<<<<droped guo xin pack, droped id: %d\n", local_task.thread_no, temp_msg.pack_no);
            continue;
        } else if(temp_msg.pack_no < last_pack) {
            printf("\n[thread: %d]<<<<<<<<<<<<<<<<<<<<get one old pack, droped id: %d\n", local_task.thread_no, temp_msg.pack_no);
            continue;
        }else if(temp_msg.pack_no == last_pack) {
            printf("\n[thread: %d]<<<<<<<<<<<<<<<<<<<<get one chong fu pack, droped id: %d\n", local_task.thread_no, temp_msg.pack_no);
            struct message ack_msg = pack_msg(ACK, temp_msg.pack_no, 0, "");
            bytes_send = sendto(sockfd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            continue;
        }

        /** got the expected pack **/
        count++;
        last_pack = temp_msg.pack_no;
        if(temp_msg.length == BODYLEN && last_pack <= end_point)
        {
            bytes_write = fwrite(temp_msg.body, sizeof(char), temp_msg.length, writefd);
            fflush(writefd);
            //printf("[thread: %d]================================ recv length: %d\n",local_task.thread_no, recv_len);
            //printf("[thread: %d]================================ write length: %d\n",local_task.thread_no, bytes_write);
            struct message ack_msg = pack_msg(ACK, temp_msg.pack_no, 0, "");
            bytes_send = sendto(sockfd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            //printf("[thread: %d] ======================== send ack byte = %d\n", bytes_send);

            /** change the global downloaded_block **/
            pthread_mutex_lock(&down_lock);
            downloaded_block++;
            //printf("[thread: %d] after get and write one =========== last_pack = %d ============ downloaded_block = %d\n", local_task.thread_no, last_pack, downloaded_block);
            pthread_mutex_unlock(&down_lock);
        }
        else if(temp_msg.length < BODYLEN )
        {
            bytes_write = fwrite(temp_msg.body, sizeof(char), temp_msg.length, writefd);
            fflush(writefd);
            //printf("[thread: %d]receive end, write byte = %d, total pack count = %d\n", local_task.thread_no, bytes_write, count);
            
            struct message ack_msg = pack_msg(ACK, temp_msg.pack_no, 0, "");
            bytes_send = sendto(sockfd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            //printf("[thread: %d] ======================== send ack byte = %d\n", bytes_send);

            pthread_mutex_lock(&down_lock);
            downloaded_block++;
            //printf("[thread: %d] ========== last_pack = %d ============= downloaded_block = %d\n", local_task.thread_no, last_pack, downloaded_block);
            pthread_mutex_unlock(&down_lock);

            //printf("[thread: %d] return from short length\n", local_task.thread_no);
            break;
        }else if (last_pack > end_point)
        {
            //printf("[thread: %d] return from overall end_point\n", local_task.thread_no);
            break;
        }


    }
    //if(recv_len <= 0)
        //printf("[thread: %d] return from receive zero length\n", local_task.thread_no);

    /** send FIN pack to server **/
    struct message fin_msg = pack_msg(FIN, 0, 0, "");
    sendto(sockfd, &fin_msg, sizeof(fin_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    fclose(writefd);
    close(sockfd);
    //pthread_cleanup_pop(1);
    //thread_cleanup_pop(1);
    return NULL;
}


/***
 ** get the new port info
 ** param body:
 ** param sockfd:
 ** param pservaddr:
 ** param servlen:
 ** rtype: response package from server
 ***/
struct synack get_new_port(struct synack body, int sockfd, struct sockaddr* pservaddr, socklen_t servlen)
{
    int recv_len = 0;
    if (connect(sockfd, (struct sockaddr*) pservaddr, servlen) == -1)
    {
        perror("connect error");
        exit(1);
    }

    struct message syn_msg = pack_msg(SYN, 0, sizeof(struct synack), &body);
    send(sockfd, &syn_msg, sizeof(struct message), 0);

    char temp_buf[MAXLEN];
    recv_len = recv(sockfd, temp_buf, MAXLEN, 0);
    if(recv_len == -1)
    {
        perror("read error");
        exit(1);
    }
    struct message ack_msg = depack_msg(temp_buf, recv_len);
    struct synack ack_pack;
    memcpy(&ack_pack, ack_msg.body, ack_msg.length);
    
    close(sockfd);
    return ack_pack;
}

/***
 ** close opened I/O stream [subthread clean function]
 ** param fd: opened I/O stream 
 ***/
void close_file(void *fd)
{
    fclose((FILE *)fd);
}

/***
 ** close opened socket [subthread clean function]
 ** param sock_hd: opened socket handler
 ***/
void close_cocket(void *sock_hd)
{
    close(*((int *)sock_hd));
}

