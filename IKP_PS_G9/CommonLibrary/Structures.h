#pragma once

#include <WinSock2.h>
#include <Windows.h>

typedef struct TopicSubscribers {
	char topic[20]; //sport
	SOCKET subsribers[10]; //pretplaceni na sport
	int size;
} TOPIC_SUBSCRIBERS;

typedef struct SubscriberQueue {
	int front, rear, size, capacity;
	TOPIC_SUBSCRIBERS* subArray; 
} SUBSCRIBER_QUEUE;

typedef struct Data {
	char topic[20];
	char message[200];
} DATA;

typedef struct MessageQueue {
	int front, rear, size, capacity;
	DATA* dataArray;
} MESSAGE_QUEUE;

