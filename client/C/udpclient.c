#include"udpclient.h"
//#include "cook_mission.h"

int repeated = 0;

void *receiver(void *arg);
void init_env();
void usage();
void process_end(int signo);
struct synack get_new_port(struct synack file_info, int sockfd, struct sockaddr* pservaddr, socklen_t servlen);
void *soldier_mission(void *task);
int popIndex(int fromState, int toState, int cycle, int thread_no);
void *cook_mission(void * arg);

int quicksort_r(char *path, char *a[],int start,int end);
int quicksort(char *path, char *fileList[], int len);
int file_modified_at(char *path, char *filename);
int file_compare1(char *path, char *filename1, char *filename2) ;
int file_compare2(char *path, char *filename, int cmpTime) ;
int list_file(char *path, int lastMaxTime);

int main(int argc, char* argv[]) {
    if(argc != 4) {
        usage();
        exit(1);
    }
    
    init_env();
    init_protocal_buf();
    pthread_mutex_init(&lockDownload, NULL);
    /** init server address **/
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("[%s] is not avalid IP address!\n", argv[1]);
        exit(1);
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    /** check if configure file exists **/
    char conf_full[100];
    strcpy(conf_full, path);
    strcat(conf_full, conf_file);
    if(access(conf_full, 0) == -1) {
        task_flag = NEW_TASK;
        printf("cannot access the file %s\n", conf_full);
        /** init config file, can be expended later for any request **/
        init_conf(conf_full);
    }else {
        task_flag = OLD_TASK;
        printf("access the file %s\n", conf_full);
        
        //TODO continue to send
        config_t *conf_hd = confRead(conf_full);
        sprintf(filename, "%s", confGet(conf_hd, "filename")); 
        load_detail(conf_hd);
        confDestory(conf_hd);
    }

    queue = Initialize_Queue();
    //Add_Queue_Item(queue, "", file_name, 0);
    //Add_Queue_Item(queue, "", file_name, 0);
    //Add_Queue_Item(queue, "", file_name, 0);

    cookLive        = YES;
    soldierLive     = YES;
    receiverLive    = YES;

    /** start Cook thread **/
    pthread_t cook_fd;
    pthread_create(&cook_fd, NULL, cook_mission, path);
    //pthread_join(cook_fd, NULL);

    /** start Receiver thread **/
    pthread_t receiver_id;
    pthread_create(&receiver_id, NULL, receiver, NULL);
    
    //pthread_join(cook_fd, NULL);
    //pthread_join(receiver_id, NULL);
    printf("===========================================================\n");

    struct synack syn_pack;
    syn_pack.thread_num = THREAD_LIMITION;
    syn_pack.file_info.blocksize = BODYLEN;


    while(clientLive){
        soldierLive = YES;
        taskIndex = 0;
        taskArray = NULL;
        filesize = 0;
        block_num = 0;

        QUEUE_ITEM *item = NULL;
        //memset(item, 0, sizeof(QUEUE_ITEM));

        while(!item && clientLive == YES){
            printf("[Main] main thread check queue\n");
            item = Get_Queue_Item(queue);
            if(item){
                printf("[Main] item.filename = %s\n", item->data);
                strcpy(filename, item->data);
                printf("[Main] transmitting: %s \n", filename);
                break;
            }else{
                printf("[Main] Queue empty will check again in 2 seconds...\n");
                sleep(2);
            }
        }
        

        strcpy(syn_pack.file_info.filename, filename);

        /** count filesize **/
        char target_full[100];
        strcpy(target_full, path);
        strcat(target_full, filename);
        int fd = open(target_full, O_RDONLY);
        filesize = get_filesize(fd);
        syn_pack.file_info.filesize = filesize;

        /** count block_num **/
        if(filesize % BODYLEN == 0)
            syn_pack.file_info.block_num = filesize / BODYLEN;
        else
            syn_pack.file_info.block_num = filesize / BODYLEN + 1;
        block_num = syn_pack.file_info.block_num;

        /** make taskArray **/
        taskArray = (int*)malloc(sizeof(int) * block_num);
        memset(taskArray, 0, sizeof(int) * block_num);

        /* count downloaded_block */
        if(task_flag == NEW_TASK) {
            printf("new task\n");
            downloaded_block = 0;
        } else {
            printf("block_num = %d, thread_num = %d, blocksize = %d, filesize = %d\n", syn_pack.file_info.block_num, syn_pack.thread_num, syn_pack.file_info.blocksize, syn_pack.file_info.filesize);

            /** count local global downloaded_block **/
            downloaded_block = 0;
            if(downloaded_block == block_num) {
                printf("download already finished\n");
                continue;
            }
            task_flag = OLD_TASK;
        }

        syn_pack.file_info.downloaded_block = downloaded_block;
        printf("figure out filesize = %d, block_num = %d, downloaded_block = %d\n", filesize, block_num, downloaded_block);

        {
        /**TODO store config info **/
        /*config_t *conf_hd = confRead(conf_full);
          if(conf_hd == NULL)
          {
          printf("conf file null, return \n");
          return 0;
          }
          confSet(conf_hd, "filename", filename);
          confSet(conf_hd, "filesize", filesize);
          store_detail(conf_hd);
          confDestory(conf_hd); */
        }

        /** ask for link with server **/
        struct synack synack_pack = get_new_port(syn_pack, sockfd, (struct sockaddr*)(&servaddr), sizeof(servaddr));
        printf("receive: thread_num = %d, downloaded_block = %d, block_num = %d\n", synack_pack.thread_num, synack_pack.file_info.downloaded_block, synack_pack.file_info.block_num);

        //if(synack_pack == NULL){
            //printf("main get connection failed, exiting...\n");
            //process_end();
            //return 0;
        //}

        /** create subthreads here **/
        task_list = (struct subthread_task *)malloc(sizeof(struct subthread_task) * THREAD_LIMITION);
        int i;
        for(i = 0; i < THREAD_LIMITION; i++) {
            task_list[i].hostname = argv[1];
            task_list[i].port = synack_pack.port[i];

            task_list[i].thread_no = i + 1;
            task_list[i].thread_num = THREAD_LIMITION;

            strcpy(task_list[i].file_info.filename, filename);
            task_list[i].file_info.filesize= filesize;

            task_list[i].file_info.block_num = block_num;
            task_list[i].file_info.blocksize= BODYLEN;

            pthread_create(&ptid[i], NULL, soldier_mission, &task_list[i]);
        }

        signal(SIGINT, process_end);
        signal(SIGKILL, process_end);

        /** print the transmit progress **/
        do{
            sleep(0.1);
            //int percent = (downloaded_block * 100) / block_num;
            /** print_process(percent, 100); **/
        }while(downloaded_block < block_num);

        //soldierLive = NO;   
        
        for(i = 0; i < THREAD_LIMITION; i ++){
            pthread_join(ptid[i], NULL);
        }
        print_process(100, 100); 
        printf("\ndownload finished, exiting...[please wait and not interrupt or power off]\n");
        
        //int j = 0;
        //for(; j < block_num; j ++)
            //printf("%d, ", taskArray[i]);
        //printf("\n");

        free(task_list);
        free(taskArray);
        task_list = NULL;
        taskArray = NULL;
    }

    pthread_mutex_destroy(&lockDownload);
    return 0;
}

/***
 ** subthread main program
 ***/
void *soldier_mission(void *task) {
    /** set subthread unable cancel **/
    //int setcancel_rst;
    //int oldstate = 0;
    //int oldtype = 0;
    //setcancel_rst = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    //setcancel_rst = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);

    struct subthread_task local_task = *((struct subthread_task *)task);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(local_task.port);
    if(inet_pton(AF_INET, local_task.hostname, &servaddr.sin_addr) <= 0) {
        printf("[%s] is not avalid IP address!\n", local_task.hostname);
        return NULL;
    }
    if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) {
        perror("connect error");
        return NULL;
    }
    
    struct msg_synack syn_msg;
    struct synack syn_pack;

    syn_pack.downloadable = 1;
    syn_pack.thread_num = THREAD_LIMITION;
    syn_pack.file_info = local_task.file_info;

    syn_msg = pack_msg_synack(SYN, 0, sizeof(syn_pack), syn_pack);
    send(sockfd, &syn_msg, sizeof(struct msg_synack), 0);

    /** set socket timeout time **/
    struct timeval tv;
    tv.tv_sec = SOCK_TIMEOUT_SEC;
    tv.tv_usec = SOCK_TIMEOUT_USEC;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        printf("socket option  SO_RCVTIMEO not support\n");
        close(sockfd);
        return NULL;
    }

    /*receive syn_ack from sever*/
    int times = 0;
    char temp_buf[MAXLEN];
    struct synack synack_pack;
    while(soldierLive == YES){
        int recv_len = recv(sockfd, temp_buf, sizeof(struct msg_synack) , 0);
        if(recv_len == -1) {
            printf("[thread: %d]timeout or read error\n", local_task.thread_no);
            send(sockfd, &syn_msg, sizeof(struct msg_synack), 0);
            if(++times > 3) {
                printf("[thread: %d] failed to receive syn_ack, transmit abort !\n", local_task.thread_no);
                return NULL;
            }
            continue;
        }
        struct message temp_msg = depack_msg(temp_buf, recv_len);
        if(temp_msg.type == SYN_ACK) {
            memcpy(&synack_pack, temp_msg.body, temp_msg.length);
            printf("[thread: %d] get syn_ack\n", local_task.thread_no);
            break;
        }else {
            printf("[thread: %d] received type: %d, not syn_ack we wanted, receive again\n", local_task.thread_no, temp_msg.type);
            continue;
        }
    }
    
    //setcancel_rst = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    //setcancel_rst = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    //pthread_cleanup_push(close_cocket, &sockfd);

    int bytes_send = 0;
    int index = 0;
    int selfLive = YES;

    char target_full[100];
    strcpy(target_full, path);
    strcat(target_full, local_task.file_info.filename);

    FILE *targHandler = fopen(target_full, "rb");
    if(targHandler == NULL){
        printf("[thread: %d] open target file failed!\n", local_task.thread_no);
        return NULL;
    }
    while(soldierLive == YES && selfLive == YES){
        index = popIndex(NEW, PENDING, 1, local_task.thread_no);
        if(index == -1){
            printf("[thread: %d] reach the end index, exiting\n", local_task.thread_no);
            break;
        }
        char body[BODYLEN];
        int seekNum = index * BODYLEN;

        int seek_rst = fseek(targHandler, seekNum, SEEK_SET);
        int bytes_read = fread(body, sizeof(char), BODYLEN, targHandler);
        /*printf("[thread: %d] read length = %d, seek_rst = %d\n", local_task.thread_no, bytes_read, seek_rst);*/

        struct message ack_msg = pack_msg(DATA, index, bytes_read, body);
        bytes_send = sendto(sockfd, &ack_msg, sizeof(struct message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        //printf("[thread: %d] ========= send DATA index = %d, bytes = %d, \n", local_task.thread_no, index, bytes_send);
    }

    //printf("will enter second cycle in 5s ... \n");
    //sleep(5);
    ///* second cycle check */
    //while(soldierLive == YES && selfLive == YES){
        //if(index >= block_num){
            ////printf("[thread: %d] reach the end index, exiting\n", local_task.thread_no);
            //continue;
        //}
        //index = popIndex(PENDING, OLD_TASK, 2, local_task.thread_no);
        //char body[BODYLEN];
        //int seekNum = index * BODYLEN;

        //int seek_rst = fseek(targHandler, seekNum, SEEK_SET);
        //int bytes_read = fread(body, sizeof(char), BODYLEN, targHandler);

        //struct message ack_msg = pack_msg(DATA, index, bytes_read, body);
        //bytes_send = sendto(sockfd, &ack_msg, sizeof(struct message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        //printf("[thread: %d] ========= send DATA index = %d, bytes = %d, \n", local_task.thread_no, index, bytes_send);
    //}

    /** send FIN pack to server **/
    struct synack fin_pack ;
    struct msg_synack fin_msg = pack_msg_synack(FIN, 0, sizeof(struct synack), fin_pack);
    sendto(sockfd, &fin_msg, sizeof(struct msg_synack), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("[thread: %d] send FIN to finish transmition...\n", local_task.thread_no);

    fclose(targHandler);
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
struct synack get_new_port(struct synack synack_pack, int sockfd, struct sockaddr* pservaddr, socklen_t servlen)
{
    int recv_len = 0;
    if (connect(sockfd, (struct sockaddr*) pservaddr, servlen) == -1) {
        perror("connect error");
        exit(1);
    }

    struct msg_synack syn_msg = pack_msg_synack(SYN, 0, sizeof(struct synack), synack_pack);
    send(sockfd, &syn_msg, sizeof(struct msg_synack), 0);

    char temp_buf[MAXLEN];
    recv_len = recv(sockfd, temp_buf, MAXLEN, 0);
    if(recv_len == -1) {
        perror("read error");
        exit(1);
    }
    struct message ack_msg = depack_msg(temp_buf, recv_len);
    struct synack ack_pack;
    memcpy(&ack_pack, ack_msg.body, ack_msg.length);
    
    return ack_pack;
}

/* taskIndex manager, in charge of search for the target task, and return its index */
int popIndex(int fromState, int toState, int cycle, int thread_no){
    int index = -1;
    static int flag = 0;
    static int last_index = 0;
    if(last_index == -1) return -1;
    
    pthread_mutex_lock(&lockSoldier);
    if(flag == 1){
        printf("[thread: %d] loop one cycle...last_index = %d\n", thread_no, last_index);
        sleep(5);
        flag = 0;
        last_index = -1;
    }
    pthread_mutex_unlock(&lockSoldier);

    pthread_mutex_lock(&lockIndex);
    for(; taskIndex < block_num; taskIndex++){
        if(taskArray[taskIndex] != DONE){
            //printf("[thread: %d][second cycle] find taskArray[%d] state %d\n", thread_no, taskIndex, taskArray[taskIndex]);
            index = taskIndex;
            last_index ++;
            taskIndex ++;
            break;
        }
    }

    if(taskIndex >= block_num){
        taskIndex = 0;
        flag = 1;
    }
    pthread_mutex_unlock(&lockIndex);

    return index;
}


// store taskArray to configure file
void store_detail(config_t *myconf){
    printf("here block_num = %d\n", block_num);
    char *str = (char*)malloc(sizeof(char) * block_num);
    printf("here\n");
    char *p = str;
    int i = 0;
    for(; i < block_num; i++){
        sprintf(p += strlen(p), "%d", taskArray[i]);
    }
    confSet(myconf, "detail", str); 
    confWrite(myconf);
    free(str);
}

// load taskArray from configure file
void load_detail(config_t *myconf){
    char * p = confGet(myconf, "detail");
    int i = 0;
    for(; i < block_num; i++)
        taskArray[i] = p[i] - '0';
}

/***
 ** Ctrl+C signal dealing function
 ** param signal: signal number
 ***/
void process_end(int signo) {
    printf("Here get 'END THE PROCESS' signal, ending subthreads...\n");
    soldierLive  = NO;
    clientLive   = NO;
    receiverLive = NO;
    //int i;
    //for(i = 0; i < THREAD_LIMITION; i++)
        //pthread_cancel(ptid[i]);

    if(!queue) Free_Queue(queue);
    if(!task_list) free(task_list);
    if(!taskArray) free(taskArray);
    exit(1);
}

/***
 ** print the usage
 ***/
void usage() {
    printf("USAGE: client hostname port filename\n");
}


int list_file(char *path, int lastMaxTime){
    char *infile[50];
    int newMaxTime = lastMaxTime;
    int length = 0;
    struct dirent *ptr;
    DIR *dir;

    dir=opendir(path);
    if(dir == NULL){ 
        printf("[Cook] open dir error");
        return lastMaxTime;
    }

    while((ptr=readdir(dir))!=NULL) {
        //跳过'.'和'..'两个目录
        if(ptr->d_name[0] == '.' || ptr->d_name[9] == 'l')
            continue;
        int cmpRst = file_compare2(path, ptr->d_name, lastMaxTime);
        if(cmpRst > 0){
            infile[length] = (char*)calloc(strlen(ptr->d_name) + 1, sizeof(char));
            sprintf(infile[length], "%s", ptr->d_name);
            printf("[Cook] adding file %s\n",infile[length]); 
            length++;

            if(newMaxTime < cmpRst)
                newMaxTime = cmpRst;
            printf("[Cook] change newMaxTime to %d\n", newMaxTime);
        }
    }

    quicksort(path, infile, length);

    int j = 0;

    //TODO add to global queue here
    printf("[Cook] provides %d new files\n", length);
    for(; j < length - 1; j++){
        Add_Queue_Item(queue, "", infile[j], strlen(infile[j]) + 1);
        free(infile[j]);
    }
    free(infile[length - 1]);

    closedir(dir);
    return newMaxTime;
}

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_ST_BIRTHTIME
#define birthtime(x) x.st_birthtime
#else
#define birthtime(x) x.st_ctime
#endif
int file_compare1(char *path, char *filename1, char *filename2) {
    struct stat st1;
    struct stat st2;

    char file1[100];
    strcpy(file1, path);
    strcat(file1, filename1); 
    
    char file2[100];
    strcpy(file2, path);
    strcat(file2, filename2);
    
    if(stat(file1, &st1) != 0)
        perror(file1);
    if(stat(file2, &st2) != 0)
        perror(file2);
    
    //printf("%s = %d, %s = %d\n", filename1, birthtime(st1), filename2, birthtime(st2));

    return birthtime(st1) >= birthtime(st2)? 1 : -1;
}   

int file_compare2(char *path, char *filename, int cmpTime) {
    struct stat st;

    char file[100];
    strcpy(file, path);
    strcat(file, filename);

    if(stat(file, &st) != 0){
        perror(file);
        return -1;
    }
    int changeTime = birthtime(st);
    //printf("%s = %d\n", filename, changeTime);

    return changeTime >= cmpTime ? changeTime : -1;
}

int file_modified_at(char *path, char *filename){
    struct stat st;

    char file[100];
    strcpy(file, path);
    strcat(file, filename);

    if(stat(file, &st) != 0){
        perror(file);
        return -1;
    }
    return birthtime(st);
}


int quicksort(char *path, char *fileList[], int len){
    quicksort_r(path, fileList, 0,len-1);
    return 0;
}
int quicksort_r(char *path, char *a[],int start,int end){
    if (start>=end) {
        return 0;
    }
    char *pivot = a[end];
    char *swp;
    //set a pointer to divide array into two parts
    //one part is smaller than pivot and another larger
    int pointer = start;
    int i;
    for (i = start; i < end; i++) {
        if (file_compare1(path, a[i], pivot) < 0){
            if (pointer != i) {
                //swap a[i] with a[pointer]
                //a[pointer] behind larger than pivot
                swp = a[i];
                a[i] = a[pointer];
                a[pointer] = swp;
            }
            pointer++;
        }
    }
    //swap back pivot to proper position
    swp = a[end];
    a[end] = a[pointer];
    a[pointer] = swp;
    quicksort_r(path, a, start,pointer - 1);
    quicksort_r(path, a,pointer + 1,end);
    return 0;
}

void *cook_mission(void * arg){
    int lastMaxTime = 0;
    while(cookLive == YES){
        //Print_Queue_Item(queue);
        int newMaxTime = list_file((char*)arg, lastMaxTime);
        if(newMaxTime == lastMaxTime){
            printf("[Receiver] till now repeated pack num = %d\n", repeated);
            printf("[Cook] no new file , will check again in 2 sec...\n");
            sleep(2);
        }else{
            lastMaxTime = newMaxTime;
            printf("[Cook] Cook list Done, lastMaxTime changed to %d\n", newMaxTime);
        }
    }
    return NULL;
}
void init_env(){
    clientLive      = YES;
    soldierLive     = YES;
    cookLive        = YES;
    receiverLive    = YES;

    taskIndex = 0; //global index share by all subthreads

    strcpy(conf_file, ".diag_log.conf");
    //strcpy(path, "/home/west/Videos/file/");
    strcpy(path, "/sdcard/Download/");

    downloaded_block = 0;
    block_num        = 0;
    filesize         = 0;

    queue     = NULL;
    taskArray = NULL;
    task_list = NULL;

}


void *receiver(void *arg){
    printf("here in receiver\n");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(RECV_ACK_PORT);
    bzero(&(servaddr.sin_zero),8);

    /** set socket timeout time 1s **/
    struct timeval tv; 
    tv.tv_sec  = SOCK_TIMEOUT_SEC + 10; 
    tv.tv_usec = SOCK_TIMEOUT_USEC;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {   
        printf("[Receiver]socket option  SO_RCVTIMEO not support\n");
        return 0;
    }

    /** bind address and port to socket **/
    if (bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) 
    {   
        perror("bind error");
        return NULL;
    }   

    printf("[Receiver] open port suc\n");
    /*receive data from sever*/
    int times;
    char temp_buf[MAXLEN];
    while(receiverLive == YES){
        int recv_len = recv(sockfd, temp_buf, sizeof(struct msg_synack), 0);
        if(recv_len == -1) {
            printf("[Receiver] timeout to wait ACK errno = %d\n", errno);
            continue;
        }

        struct message temp_msg = depack_msg(temp_buf, recv_len);
        int index = temp_msg.pack_no;
        if(temp_msg.type == ACK){         // got one ack package
            if(!taskArray){
                printf("[Receiver] taskArray not available ...\n");
                continue;
            }
            pthread_mutex_lock(&lockIndex);
            if(taskArray[index] != DONE){ 
                taskArray[index] = DONE;
                pthread_mutex_unlock(&lockIndex);

                /** change the global downloaded_block **/
                pthread_mutex_lock(&lockDownload);
                int db = ++downloaded_block;
                pthread_mutex_unlock(&lockDownload);
                //printf("[Receiver] ========= get ACK for index = %d ======== downloaded_block = %d\n", index, db);
            }else {
                pthread_mutex_unlock(&lockIndex);
                printf("[Receiver] get old ack, index = %d\n", index);
                repeated ++;
                continue;
            }
        }else if(temp_msg.type == FIN){
            printf("[Receiver] return from FIN\n");
            //receiverLive = NO;
            soldierLive  = NO;
        }else {
            printf("[Receiver] get unknow type: %d\n", temp_msg.type);
            continue;
        }
    }
    return NULL;
}

