#pragma once
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "../CommonLibrary/Queue.h"
#include "../CommonLibrary/Structures.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define DEFAULT_CLIENT_PORT 27017
#define SERVER_SLEEP_TIME 50
#define NUMBER_OF_CLIENTS 40
#define INV_SOCKET 3435973836

int numberOfPublishers = 0;
THREAD_ARGUMENT publisherThreadArgument;
bool server_running = true;

void Connect(SOCKET);
int SelectFunction(SOCKET, char);
char* ReceiveFunction(SOCKET);
void Publish(MESSAGE_QUEUE*, char*, char*, int);
int SendFunction(SOCKET connectSocket, char* message, int messageSize);

void Connect(SOCKET acceptedSocket) {

	publisherThreadArgument.clientNumber = numberOfPublishers;
	publisherThreadArgument.socket = acceptedSocket;

	printf("\nPublisher %d connected.\n", ++numberOfPublishers);
}

int SelectFunction(SOCKET listenSocket, char rw) {
	int iResult = 0;
	do {
		FD_SET set;
		struct timeval timeVal;

		FD_ZERO(&set);

		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (!server_running)
			return -1;

		if (rw == 'r') {
			iResult = select(0, &set, NULL, NULL, &timeVal);
		}
		else {
			iResult = select(0, NULL, &set, NULL, &timeVal);
		}


		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "\nselect failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		if (iResult == 0)
		{
			Sleep(SERVER_SLEEP_TIME);
			continue;
		}
		break;

	} while (1);

	return 1;
}

char* ReceiveFunction(SOCKET acceptedSocket) {

	int iResult;
	char* myBuffer = (char*)(malloc(DEFAULT_BUFLEN));

	if (myBuffer == NULL)
	{
		printf("Unable to allocate memory for the BUFFER");
		exit(0);
	}

	int selectResult = SelectFunction(acceptedSocket, 'r');
	if (selectResult == -1) {
		memcpy(myBuffer, "ErrorS", 7);
		return myBuffer;
	}
	iResult = recv(acceptedSocket, myBuffer, 256, 0);

	if (iResult > 0)
	{
		myBuffer[iResult] = '\0';
	}
	else if (iResult == 0)
	{
		memcpy(myBuffer, "ErrorC", 7);
	}
	else
	{
		memcpy(myBuffer, "ErrorR", 7);
	}
	return myBuffer;
}

void Publish(MESSAGE_QUEUE* messageQueue, char* topic, char* message, int clientNumber) {

	DATA data;
	memcpy(data.message, message, strlen(message) + 1);
	memcpy(data.topic, topic, strlen(topic) + 1);

	EnqueueMessage(messageQueue, data);

	printf("\nPublisher %d published new message to topic %s.\nMessage: %s\n", clientNumber + 1, data.topic, data.message);
}


int SendFunction(SOCKET connectSocket, char* message, int messageSize) {

	int selectResult = SelectFunction(connectSocket, 'w');
	if (selectResult == -1) {
		return -1;
	}
	int iResult = send(connectSocket, message, messageSize, 0);

	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 0;
	}

	return 1;
}
