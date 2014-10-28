#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"

QUEUE *Initialize_Queue(void)
{
	QUEUE *q;
	
	q = calloc(1, sizeof(QUEUE));
	if (!(q))
		return NULL;
    q->head = NULL;
    q->tail = q->head;
	
	pthread_mutex_init(&(q->modify_mutex), NULL);
	
	return q;
}

void Append_Queue_Item(QUEUE *queue, char* filename, int pack_no, void *data, size_t length)
{
	QUEUE_ITEM *qi;
	
	qi = calloc(1, sizeof(QUEUE_ITEM));
	if (!(qi)){
        printf("[QUEUE] calloc failed\n");
        return;
    }

    int filename_length = strlen(filename);
    qi->filename = calloc(filename_length + 1, sizeof(char));
    memcpy(qi->filename, filename, filename_length + 1);

	qi->data = calloc(length + 1, sizeof(char));
    memcpy(qi->data, data, length + 1);
    //printf("from Add_Queue_Item, filename : %s \n", qi->data);
	qi->length = length;
	qi->pack_no = pack_no; 
	
	pthread_mutex_lock(&(queue->modify_mutex));
	
    if(queue->head == NULL){
        queue->head = qi;
        queue->tail = qi;
    }else{
        queue->tail->next = qi;
        queue->tail = qi;
        queue->tail->next = NULL;
    }

	queue->numitems++;
	
	pthread_mutex_unlock(&(queue->modify_mutex));
}

QUEUE_ITEM *Pop_Queue_Item(QUEUE *queue) {
    QUEUE_ITEM *pqi;

    pthread_mutex_lock(&(queue->modify_mutex));

    if(!(pqi = queue->head)) {
        pthread_mutex_unlock(&(queue->modify_mutex));
        return (QUEUE_ITEM *)NULL;
    } 

	pqi = queue->head;
    queue->head = queue->head->next;
	pqi->next = NULL;
	queue->numitems--;
	
	pthread_mutex_unlock(&(queue->modify_mutex));
	
    //printf("from Pop_Queue_Item, data = %s\n", pqi->data);
	return pqi;
}

void Print_Queue_Item(QUEUE *queue) {
    QUEUE_ITEM *qi;

    pthread_mutex_lock(&(queue->modify_mutex));

    if(!(qi = queue->head)) {
        pthread_mutex_unlock(&(queue->modify_mutex));
        printf("[In Queue] Queue empty!\n");
        return NULL;
    } 

	qi = queue->head;
	while (qi){
        printf("[In Queue] %s\n", qi->data);
		qi = qi->next;
	}
    printf("[In Queue] Queue length = %d\n", queue->numitems);
	
    //if (queue->numitems > 0)
	pthread_mutex_unlock(&(queue->modify_mutex));
	return NULL;
}

void Free_Queue_Item(QUEUE_ITEM *queue_item)
{
	free(queue_item->data);
	free(queue_item->filename);
    queue_item->next = NULL;
	free(queue_item);
}

void Free_Queue(QUEUE *queue)
{
    QUEUE_ITEM *item;

    while (queue->head) {
        item = Pop_Queue_Item(queue);
        fprintf(stderr, "(filename: %s)(paack_no: %d): (length: %u)\n", item->filename, item->pack_no, item->length);
        Free_Queue_Item(item);
    }
}

//#if defined(TEST_CODE)
//int main(int argc, char *argv[])
//{
    //QUEUE *queue;
    //QUEUE_ITEM *items, *item;

    //queue = Initialize_Queue();
    //Append_Queue_Item(queue, "hello", 1, "data 1", sizeof("data 1"));
    //Append_Queue_Item(queue, "hello", 2, "data 2", sizeof("data 2"));
    //Append_Queue_Item(queue, "hello", 3, "data 3", sizeof("data 3"));
    //Append_Queue_Item(queue, "hello", 4, "data 4", sizeof("data 4"));
    //Append_Queue_Item(queue, "hello", 5, "data 5", sizeof("data 5"));

    //Free_Queue(queue);

    //return 0;
//}
//#endif
