#include "udpserver.h"

void *writer(){
    
    QUEUE_ITEM *item;
    int pack_no = 0;
    FILE* file_fd;

    while (writer_live > 0) {
        item = Pop_Queue_Item(data_queue);
        if(!item){
            printf("[Manager] find nothing to write\n");
            sleep(1);
            continue;
        }
        
        if(access(item->filename, 0) == -1) 
        {   
            FILE *temp_handler = fopen(item->filename, "w+");
            fclose(temp_handler);
        }

        file_fd= fopen(item->filename, "rb+");
        if(file_fd == NULL) {
            printf("[Manager]open file fialed\n");
            return NULL;
        }
        //printf("[Manager]open file %s successfully, fileno = %d\n",item->filename, fileno(file_fd));

        int seek_num = item->pack_no * BODYLEN; 
        int seek_result = fseek(file_fd, seek_num, SEEK_SET);
        //printf("[Manager]seek_num = %d, seek result = %d, \n", seek_num, seek_result);
        int bytes_write = fwrite(item->data, sizeof(char), item->length, file_fd);
        fflush(file_fd);

        //printf("[Manager] recv ===== DATA pack_no: %d ====== length = %d ====== write bytes = %d\n", item->pack_no, item->length, bytes_write);

        fclose(file_fd);

        Free_Queue_Item(item);
    }
}

//int main(){
    //data_queue = Initialize_Queue();
    //Append_Queue_Item(data_queue, "hello", 0, "data 1", sizeof("data 1"));
    //Append_Queue_Item(data_queue, "hello", 1, "data 2", sizeof("data 2"));
    //Append_Queue_Item(data_queue, "hello", 2, "data 3", sizeof("data 3"));
    //Append_Queue_Item(data_queue, "hello", 3, "data 4", sizeof("data 4"));
    //Append_Queue_Item(data_queue, "hello", 4, "data 5", sizeof("data 5"));

    //writer();
//}
