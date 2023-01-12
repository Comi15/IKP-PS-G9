#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include "Subscriber.h"

HANDLE SendThread, RecvThread;
DWORD SendThreadId, RecvThreadId;

DWORD WINAPI SubscriberSend(LPVOID lpParam) {

	int iResult = 0;
	int topics[6];
	int topicCount = 0;

	SOCKET connectSocket = *(SOCKET*)lpParam;
	while (sub_running) {

		PrintMenu();
		char input = _getch();

		char* message = (char*)malloc(20 * sizeof(char));

		if (message == NULL) {
			printf("Unable to allocate memory for message.");
			exit(0);
		}

		if (input == '1' || input == '2' || input == '3' || input == '4' || input == '5' || input == '6') {

			if (AlreadySubscribed(input, topics, topicCount)) {
				printf("You are already subscribed to this topic.\n");
				continue;
			}

			topics[topicCount++] = input - '0';

			ProcessInput(input, message);

			int messageSize = strlen(message) + 1;
			int sendResult = SendFunction(connectSocket, (char*)message, messageSize);

			free(message);
			if (sendResult == -1) {
				return 1;
			}
		}
		else if (input == 'x' || input == 'X') {
			strcpy(message, "shutdown");

			int messageSize = strlen(message) + 1;
			int sendResult = SendFunction(connectSocket, (char*)message, messageSize);

			free(message);
			if (sendResult == -1) {
				break;
			}

			sub_running = false;
			break;
		}
		else {
			printf("Invalid input.\n");
			free(message);
			continue;
		}
	}
	return 1;
}

DWORD WINAPI SubscriberReceive(LPVOID lpParam) {
	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	char* recvRes;

	while (sub_running)
	{
		recvRes = ReceiveFunction(connectSocket);

		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			char delimiter[] = ":";

			char* ptr = strtok(recvRes, delimiter);

			char* topic = ptr;
			ptr = strtok(NULL, delimiter);
			char* message = ptr;
			ptr = strtok(NULL, delimiter);

			printf("\nReceived new message to topic %s.\nMessage: %s\n", topic, message);

			free(recvRes);
		}
		else if (!strcmp(recvRes, "ErrorS")) {
			closesocket(connectSocket);
			sub_running = false;
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorC"))
		{
			printf("\nConnection with server closed.\n");
			printf("Press any key to close this window...");
			closesocket(connectSocket);
			sub_running = false;
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			sub_running = false;
			free(recvRes);
			break;
		}
	}
	return 1;
}

int main()
{
	SOCKET connectSocket = INVALID_SOCKET;
	int iResult;

	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(DEFAULT_PORT);

	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}

	unsigned long int nonBlockingMode = 1;
	iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);

	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	Connect(connectSocket);
	SendThread = CreateThread(NULL, 0, &SubscriberSend, &connectSocket, 0, &SendThreadId);
	RecvThread = CreateThread(NULL, 0, &SubscriberReceive, &connectSocket, 0, &SendThreadId);

	while (sub_running) {

	}

	if (SendThread)
		WaitForSingleObject(SendThread, INFINITE);

	if (RecvThread)
		WaitForSingleObject(RecvThread, INFINITE);

	SAFE_DELETE_HANDLE(SendThread);
	SAFE_DELETE_HANDLE(RecvThread);
	
	closesocket(connectSocket);

	WSACleanup();

	return 0;
}