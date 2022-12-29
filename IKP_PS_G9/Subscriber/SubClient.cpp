#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include "Subscriber.h"

HANDLE SubscriberSendThread, SubscriberRecvThread;
DWORD SubscriberSendThreadId, SubscriberRecvThreadId;

DWORD WINAPI SubscriberSend(LPVOID lpParam) {

	int iResult = 0;
	SOCKET connectSocket = *(SOCKET*)lpParam;
	while (sub_running) {

		PrintMenu();
		char input = _getch();

		char* message = (char*)malloc(20 * sizeof(char));

		if (input == '1' || input == '2' || input == '3' || input == '4' || input == '5' || input == '6') {

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
	SubscriberSendThread = CreateThread(NULL, 0, &SubscriberSend, &connectSocket, 0, &SubscriberSendThreadId);

	while (sub_running) {

	}

	if (SubscriberSendThread)
		WaitForSingleObject(SubscriberSendThread, INFINITE);

	SAFE_DELETE_HANDLE(SubscriberSendThread);
	
	closesocket(connectSocket);

	WSACleanup();

	return 0;
}