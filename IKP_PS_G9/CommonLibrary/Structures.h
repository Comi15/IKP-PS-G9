#pragma once

#include <WinSock2.h>
#include <Windows.h>

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define TOPIC_LEN 20

typedef struct TopicSubscribers {
	int size;
	char* topic; //sport
	SOCKET subsribers[10]; //pretplaceni na sport
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

typedef struct ThreadArgument {
	SOCKET socket;
	int clientNumber;
} THREAD_ARGUMENT;