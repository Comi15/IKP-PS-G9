#pragma once

#include <stdio.h>
#include <WinSock2.h>
#include "Structures.h"

//red u kom se skladiste subsrciberi
SUBSCRIBER_QUEUE* CreateSubQueue(int capacity) {
	SUBSCRIBER_QUEUE* queue = (SUBSCRIBER_QUEUE*)malloc(sizeof(SUBSCRIBER_QUEUE));
	if (queue == NULL) {
		printf("Unable to allocate memory for subsriber queue.");
		exit(0);
	}

	queue->capacity = capacity;
	queue->size = 0;
	queue->front = queue->size;
	queue->rear = capacity - 1;
	queue->subArray = (TOPIC_SUBSCRIBERS*)malloc(queue->capacity * sizeof(TOPIC_SUBSCRIBERS));
	return queue;
}

//funkcija koja kreira red u kom se skladiste poruke koje publisher salje na odredjenu temu
MESSAGE_QUEUE* CreateMessageQueue(int capacity) {
	MESSAGE_QUEUE* queue = (MESSAGE_QUEUE*)malloc(sizeof(MESSAGE_QUEUE));
	if (queue == NULL) {
		printf("Unable to allocate memory for message queue.");
		exit(0);
	}

	queue->capacity = capacity;
	queue->size = 0;
	queue->front = queue->size;
	queue->rear = capacity - 1;
	queue->dataArray = (DATA*)malloc(queue->capacity * sizeof(DATA));
	return queue;
}

int IsSubQueueFull(SUBSCRIBER_QUEUE* queue) {
	if (queue->size == queue->capacity)
		return 1;
	else
		return 0;
}

int IsMessageQueueFull(MESSAGE_QUEUE* queue) {
	if (queue->size == queue->capacity)
		return 1;
	else
		return 0;
}

int IsSubQueueEmpty(SUBSCRIBER_QUEUE* queue) {
	if (queue->size == 0)
		return 1;
	else
		return 0;
}

int IsMessageQueueEmpty(MESSAGE_QUEUE* queue) {
	if (queue->size == 0)
		return 1;
	else
		return 0;
}

//pravi red sa 5 (???) struktura topic subscribera 
void EnqueueSub(SUBSCRIBER_QUEUE* queue, char* topic) {
	TOPIC_SUBSCRIBERS ts;
	strcpy(ts.topic, topic);
	ts.size = 0; 

	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->subArray[queue->rear] = ts;
	queue->size = queue->size + 1;
	printf("%s enqueued to subscriber queue.\n", ts.topic);
}

void EnqueueMessage(MESSAGE_QUEUE* queue, DATA data) {

	queue->rear = (queue->rear + 1) % queue->capacity;
	queue->dataArray[queue->rear] = data;
	queue->size = queue->size + 1;
	//printf("%s enqueued to message queue.\n", data.topic);
}

TOPIC_SUBSCRIBERS DequeueSub(SUBSCRIBER_QUEUE* queue) {
	if (!IsSubQueueEmpty(queue)) {
		TOPIC_SUBSCRIBERS ts = queue->subArray[queue->front];
		queue->front = (queue->front + 1) % queue->capacity;
		queue->size = queue->size - 1;
		return ts;
	}
}

DATA DequeueMessage(MESSAGE_QUEUE* queue) {
	if (!IsMessageQueueEmpty(queue)) {
		DATA data = queue->dataArray[queue->front];
		queue->front = (queue->front + 1) % queue->capacity;
		queue->size = queue->size - 1;
		return data;
	}
}

