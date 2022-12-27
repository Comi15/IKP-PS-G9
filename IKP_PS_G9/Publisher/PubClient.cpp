#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include "Publisher.h"

void PublisherSend(SOCKET connectSocket, char* message) {

	PrintMenu();
	char input = _getch();

	if (input == '1' || input == '2' || input == '3' || input == '4' || input == '5' || input == '6') {

		ProcessInput(input, message);

		char* publishMessage = (char*)malloc(250 * sizeof(char));

		EnterAndGenerateMessage(publishMessage, message);

		int messageSize = strlen(message) + 1;
		int sendResult = SendFunction(connectSocket, message, messageSize);

		free(publishMessage);

		if (sendResult == -1)
			return;
	}
	else if (input == 'x' || input == 'X') {
		strcpy(message, "shutdown");

		int sendResult = SendFunction(connectSocket, message, 9);

		if (sendResult == -1) {
			return;
		}
	}
	else {
		printf("Invalid input.\n");
		return;
	}
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

	char* message = (char*)malloc(250 * sizeof(char));

	while (server_running) {

		PublisherSend(connectSocket, message);

		if (!strcmp(message, "shutdown"))
			break;
	}

	free(message);
	closesocket(connectSocket);

	WSACleanup();

	return 0;
}