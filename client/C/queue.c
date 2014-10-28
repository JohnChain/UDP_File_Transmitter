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
	
	pthread_mutex_init(&(q->modify_mutex), NULL);
	
	return q;
}

void Add_Queue_Item(QUEUE *queue, char *action, void *data, size_t sz)
{
	QUEUE_ITEM *qi;
	
	qi = calloc(1, sizeof(QUEUE_ITEM));
	if (!(qi))
		return;
	

	qi->data = calloc(sz + 1, sizeof(char));
    memcpy(qi->data, data, sz + 1);
    printf("from Add_Queue_Item, fileanme : %s \n", qi->data);
	qi->sz = sz;
	qi->action = strdup(action);
	
	pthread_mutex_lock(&(queue->modify_mutex));
	
	qi->next = queue->items;
	queue->items = qi;
	queue->numitems++;
	
	pthread_mutex_unlock(&(queue->modify_mutex));
}

QUEUE_ITEM *Get_Queue_Item(QUEUE *queue) {
    QUEUE_ITEM *qi, *pqi;

    pthread_mutex_lock(&(queue->modify_mutex));

    if(!(qi = pqi = queue->items)) {
        pthread_mutex_unlock(&(queue->modify_mutex));
        return (QUEUE_ITEM *)NULL;
    } 

	qi = pqi = queue->items;
	while ((qi->next)) {
		pqi = qi;
		qi = qi->next;
	}
	
	pqi->next = NULL;
	if (queue->numitems == 1)
		queue->items = NULL;
	
	queue->numitems--;
	
    //if (queue->numitems > 0)
	pthread_mutex_unlock(&(queue->modify_mutex));
	
    printf("from Get_Queue_Item, filename = %s\n", qi->data);
	return qi;
}

void Print_Queue_Item(QUEUE *queue) {
    QUEUE_ITEM *qi;

    pthread_mutex_lock(&(queue->modify_mutex));

    if(!(qi = queue->items)) {
        pthread_mutex_unlock(&(queue->modify_mutex));
        //printf("[In Queue] Queue empty!\n");
        return NULL;
    } 

	qi = queue->items;
	while (qi) {
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
	free(queue_item->action);
	free(queue_item->data);
	free(queue_item);
}

void Free_Queue(QUEUE *queue)
{
    QUEUE_ITEM *item;

    while (queue->items) {
        item = Get_Queue_Item(queue);
        fprintf(stderr, "[*] action[%s]: %s (%u)\n", item->action, item->data, item->sz);
        Free_Queue_Item(item);
    }
}

#if defined(TEST_CODE)
int main(int argc, char *argv[])
{
    QUEUE *queue;
    QUEUE_ITEM *items, *item;

    queue = Initialize_Queue();
    //Add_Queue_Item(queue, "action 1", "data 1", sizeof("data 1"));
    //Add_Queue_Item(queue, "action 2", "data 2", sizeof("data 2"));
    //Add_Queue_Item(queue, "action 3", "data 3", sizeof("data 3"));
    //Add_Queue_Item(queue, "action 4", "data 4", sizeof("data 4"));
    //Add_Queue_Item(queue, "action 5", "data 5", sizeof("data 5"));

    Free_Queue(queue);

    return 0;
}
#endif
