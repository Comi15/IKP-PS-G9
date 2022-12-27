#pragma once
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT 27016
#define SERVER_SLEEP_TIME 50
#define MAX_MESSAGE_SIZE 250

bool server_running = true;

void PrintMenu();
int ValidateMessage(char*);
void EnterAndGenerateMessage(char*, char*);
void ProcessInput(char, char*);
int SendFunction(SOCKET, char*, int);
int SelectFunction(SOCKET, char);

void PrintMenu() {
	printf("\nChoose a topic: \n");
	printf("\t1.Animals\n");
	printf("\t2.History\n");
	printf("\t3.Geography\n");
	printf("\t4.Sport \n");
	printf("\t5.Mathematics\n");
	printf("\t6.Music \n");
	printf("Press X if you want to close connection.\n");
}

int ValidateMessage(char* publishMessage) {
	if (!strcmp(publishMessage, "\n")) {
		return 0;
	}

	int messageLength = strlen(publishMessage);

	for (int i = 0; i < messageLength - 1; i++)
	{
		if (publishMessage[i] != ' ' && publishMessage[i] != '\t') {
			return 1;
		}
	}
	return 0;
}

void EnterAndGenerateMessage(char* publishMessage, char* topic)
{
	printf("Enter message: \n");

	fgets(publishMessage, MAX_MESSAGE_SIZE, stdin);

	while (!ValidateMessage(publishMessage)) {

		printf("Message cannot be empty. Please enter your message again: \n");
		fgets(publishMessage, MAX_MESSAGE_SIZE, stdin);
	}

	if ((strlen(publishMessage) > 0) && (publishMessage[strlen(publishMessage) - 1] == '\n'))
		publishMessage[strlen(publishMessage) - 1] = '\0';

	strcat(topic, ":");
	strcat(topic, publishMessage);

	printf("You published message: %s.\n", publishMessage);
}

void ProcessInput(char input, char* message) {
	if (input == '1') {
		strcpy(message, "Animals");
	}
	else if (input == '2') {
		strcpy(message, "History");
	}
	else if (input == '3') {
		strcpy(message, "Geography");
	}
	else if (input == '4') {
		strcpy(message, "Sport");
	}
	else if (input == '5') {
		strcpy(message, "Mathematics");
	}
	else if (input == '6') {
		strcpy(message, "Music");
	}
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

int SelectFunction(SOCKET listenSocket, char rw) {

	int iResult = 0;
	do {
		FD_SET set;
		struct timeval timeVal;

		FD_ZERO(&set);

		FD_SET(listenSocket, &set);

		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (!server_running) {
			return -1;
		}

		if (rw == 'r') {
			iResult = select(0, &set, NULL, NULL, &timeVal);
		}
		else {
			iResult = select(0, NULL, &set, NULL, &timeVal);
		}


		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
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

