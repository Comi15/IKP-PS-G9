#include<stdio.h>


#include "PubSub2.h"

CRITICAL_SECTION queueAccess;
CRITICAL_SECTION message_queueAccess;
bool serverStopped = false;

HANDLE pubSubSemaphore;
int publisherThreadKilled = -1;
int subscriberSendThreadKilled = -1;
int subscriberRecvThreadKilled = -1;

SOCKET acceptedSocket;
int clientsCount = 0;

 SUBSCRIBER_QUEUE* subQueue;
 MESSAGE_QUEUE* messageQueue;
 DATA poppedMessage;
//struct Subscriber subscribers[NUMBER_OF_CLIENTS];


//HANDLE SubscriberSendThreads[NUMBER_OF_CLIENTS];
//DWORD SubscriberSendThreadsID[NUMBER_OF_CLIENTS];

//HANDLE SubscriberRecvThreads[NUMBER_OF_CLIENTS];
//DWORD SubscriberRecvThreadsID[NUMBER_OF_CLIENTS];

HANDLE PubSub2Thread;
DWORD PubSub2ThreadId;

DWORD WINAPI PubSub2Recieve(LPVOID lpParam)
{
	int iResult = 0;
	SOCKET acceptedSocket = *(SOCKET*)lpParam;
	char* recvRes;

	while (server_running) {

		recvRes = ReceiveFunction(acceptedSocket);
		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			char delimiter[] = ":";

			char* ptr = strtok(recvRes, delimiter);

			char* topic = ptr;
			ptr = strtok(NULL, delimiter);
			char* message = ptr;

			if (!strcmp(topic, "shutdown")) {
				printf("\nPubSub1 disconnected.\n");
				acceptedSocket = -1;
				free(recvRes);
				break;
			}
			else {
				ptr = strtok(NULL, delimiter);
				EnterCriticalSection(&message_queueAccess);
				Forward(messageQueue, topic, message);
				LeaveCriticalSection(&message_queueAccess);
				ReleaseSemaphore(pubSubSemaphore, 1, NULL);
				free(recvRes);
			}
		}
		else if (!strcmp(recvRes, "ErrorS")) {
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
			printf("\nConnection with client closed.\n");
			closesocket(acceptedSocket);
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			printf("\nrecv failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
			free(recvRes);
			break;

		}
	}

	return 1;
}

int main()
{
	subQueue = CreateSubQueue(10);
	messageQueue = CreateMessageQueue(1000);

	AddTopics(subQueue);

	InitializeCriticalSection(&queueAccess);
	InitializeCriticalSection(&message_queueAccess);

	pubSubSemaphore = CreateSemaphore(0, 0, 1, NULL);
	

	SOCKET listenSocket = INVALID_SOCKET;

	int iResult;

	char recvbuf[DEFAULT_BUFLEN];

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}


	struct addrinfo* resultingAddress = NULL;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("\ngetaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	listenSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		printf("\nsocket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("\nbind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &nonBlockingMode);

	if (iResult == SOCKET_ERROR)
	{
		printf("\nioctlsocket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	freeaddrinfo(resultingAddress);


	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("\nlisten failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("\nServer successfully started, waiting for clients.\n");

	while (clientsCount < NUMBER_OF_CLIENTS && server_running)
	{
		int selectResult = SelectFunction(listenSocket, 'r');
		if (selectResult == -1) {
			break;
		}

		acceptedSocket = accept(listenSocket, NULL, NULL);

		if (acceptedSocket == INVALID_SOCKET)
		{
			printf("\naccept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		char* client = Connect(acceptedSocket);
		if (!strcmp(client, "pubsub1")) {
			PubSub2Thread = CreateThread(NULL, 0, &PubSub2Recieve, &acceptedSocket, 0, &PubSub2ThreadId);
		}
		else if (!strcmp(client, "sub")) {
			//niti za subscribere
			clientsCount++;
		}
		
	}

	

	printf("\nServer shutting down...\n");


	DeleteCriticalSection(&queueAccess);
	DeleteCriticalSection(&message_queueAccess);

	

	//SAFE_DELETE_HANDLE(pubSubSemaphore);

	closesocket(listenSocket);

	free(subQueue);
	free(messageQueue->dataArray);
	free(messageQueue);

	WSACleanup();



	return 0;
}