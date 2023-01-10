#include "PubSub1.h"

CRITICAL_SECTION message_queueAccess;

HANDLE pubSubSemaphore;

SOCKET acceptedSockets[NUMBER_OF_CLIENTS];

MESSAGE_QUEUE* messageQueue;
DATA poppedMessage;

int clientsCount = 0;
bool pubsub2_running = true;

HANDLE PublisherThreads[NUMBER_OF_CLIENTS];
DWORD PublisherThreadsID[NUMBER_OF_CLIENTS];

HANDLE PubSubForwardThread;
DWORD PubSubForwardThreadID;

HANDLE StopServerThread;
DWORD StopServerThreadID;

#define SAFE_DELETE_HANDLE(h) {if(h)CloseHandle(h);}

//funkcija koja se izvrsava u niti i prima poruku od Publisher-a i stavlja je u red 
DWORD WINAPI PublisherReceive(LPVOID lpParam)
{
	int iResult = 0;
	THREAD_ARGUMENT argumentStructure = *(THREAD_ARGUMENT*)lpParam;
	char* recvRes;

	while (pubsub1_running) {

		recvRes = ReceiveFunction(argumentStructure.socket);
		if (strcmp(recvRes, "ErrorC") && strcmp(recvRes, "ErrorR") && strcmp(recvRes, "ErrorS"))
		{
			char delimiter[] = ":";

			char* ptr = strtok(recvRes, delimiter);

			char* topic = ptr;
			ptr = strtok(NULL, delimiter);
			char* message = ptr;
			
			if (!strcmp(topic, "shutdown")) {
				printf("\nPublisher %d disconnected.\n", argumentStructure.clientNumber + 1);
				acceptedSockets[argumentStructure.clientNumber] = -1;
				free(recvRes);
				break;
			}
			else {
				ptr = strtok(NULL, delimiter);
				EnterCriticalSection(&message_queueAccess);
				Publish(messageQueue, topic, message, argumentStructure.clientNumber);
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
			closesocket(argumentStructure.socket);
			free(recvRes);
			break;
		}
		else if (!strcmp(recvRes, "ErrorR"))
		{
			printf("\nrecv failed with error: %d\n", WSAGetLastError());
			closesocket(argumentStructure.socket);
			free(recvRes);
			break;

		}
	}
	
	return 1;
}

//Funkcija koja se izvrsava u niti i prosledjuje poruku primljenu od Publishera PubSubEngin-u 2
DWORD WINAPI ForwardMessage(LPVOID lpParam)
{
	int iResult = 0;
	SOCKET connectedSocket = *(SOCKET*)lpParam;

	while (pubsub1_running)
	{

		WaitForSingleObject(pubSubSemaphore, INFINITE);

		if (!pubsub1_running)
			break;

		EnterCriticalSection(&message_queueAccess);
		poppedMessage = DequeueMessage(messageQueue);
		LeaveCriticalSection(&message_queueAccess);

		char* message = (char*)malloc(sizeof(DATA) + 1);

		if (message == NULL)
		{
			printf("Unable to alocate memory for the message buffer.");
			exit(0);
		}

		memcpy(message, &poppedMessage.topic, (strlen(poppedMessage.topic)));
		memcpy(message + (strlen(poppedMessage.topic)), ":", 1);
		memcpy(message + (strlen(poppedMessage.topic) + 1), &poppedMessage.message, (strlen(poppedMessage.message) + 1));

		int messageSize = strlen(message) + 1;
		int sendResult = SendFunction(connectedSocket, message, messageSize);
		
		free(message);
		

		if (sendResult == -1 || sendResult == 0) {
			pubsub2_running = false;
			pubsub1_running = false;
			break;
		}
	}
		
	return 1;
}

DWORD WINAPI StopServer(LPVOID lpParam)
{
	char input;
	SOCKET connectedSocket = *(SOCKET*)lpParam;

	while (pubsub1_running) {

		printf("\nPress X to stop server.\n");
		input = _getch();

		if (input == 'x' || input == 'X') {

			int iResult = 0;
			iResult = SendFunction(connectedSocket, (char*)"shutdown", 9);
			pubsub1_running = false;

			ReleaseSemaphore(pubSubSemaphore, 1, NULL);

			for (int i = 0; i < clientsCount; i++) {
				if (acceptedSockets[i] != -1) {
					iResult = shutdown(acceptedSockets[i], SD_BOTH);
					if (iResult == SOCKET_ERROR)
					{
						printf("\nshutdown failed with error: %d\n", WSAGetLastError());
						closesocket(acceptedSockets[i]);
						return 1;
					}
					closesocket(acceptedSockets[i]);
				}
			}
			closesocket(connectedSocket);

			break;
		}
	}
	return 1;
}

int main() 
{
	messageQueue = CreateMessageQueue(1000);

	InitializeCriticalSection(&message_queueAccess);

	pubSubSemaphore = CreateSemaphore(0, 0, 1, NULL);

	SOCKET listenSocket = INVALID_SOCKET;

	int iResult;

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



	//Kreiranje klijentskog socket-a za komunikaciju sa PubSubEngine2 komponentom
	SOCKET connectSocket = INVALID_SOCKET;


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
	serverAddress.sin_port = htons(DEFAULT_CLIENT_PORT);

	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
	}

	
	int iResultSend = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);

	if (iResultSend == SOCKET_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	printf("\nServer successfully started, waiting for clients.\n");

	ConnectToPubSub2(connectSocket);
	
	PubSubForwardThread = CreateThread(NULL, 0, &ForwardMessage, &connectSocket, 0, &PubSubForwardThreadID);
	StopServerThread = CreateThread(NULL, 0, &StopServer, &connectSocket, 0, &StopServerThreadID);

	while (clientsCount < NUMBER_OF_CLIENTS && pubsub2_running)
	{
		int selectResult = SelectFunction(listenSocket, 'r');
		if (selectResult == -1) {
			break;
		}

		acceptedSockets[clientsCount] = accept(listenSocket, NULL, NULL);

		if (acceptedSockets[clientsCount] == INVALID_SOCKET)
		{
			printf("\naccept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		Connect(acceptedSockets[clientsCount]);
		PublisherThreads[clientsCount] = CreateThread(NULL, 0, &PublisherReceive, &publisherThreadArgument, 0, &PublisherThreadsID[clientsCount]);
		
		clientsCount++;
	}

	for (int i = 0; i < clientsCount; i++) {
	
		if (PublisherThreads[i])
			WaitForSingleObject(PublisherThreads[i], INFINITE);
	}

	if (PubSubForwardThread)
	{
		WaitForSingleObject(PubSubForwardThread, INFINITE);
	}

	if (StopServerThread) {
		WaitForSingleObject(StopServerThread, INFINITE);
	}

	printf("\nServer shutting down...\n");
	
	DeleteCriticalSection(&message_queueAccess);

	for (int i = 0; i < clientsCount; i++) {
		SAFE_DELETE_HANDLE(PublisherThreads[i]);
	}
	
	SAFE_DELETE_HANDLE(PubSubForwardThread);
	SAFE_DELETE_HANDLE(StopServerThread);

	SAFE_DELETE_HANDLE(pubSubSemaphore);
	
	closesocket(listenSocket);
	closesocket(connectSocket);

	free(messageQueue->dataArray);
	free(messageQueue);

	WSACleanup();


	return 0;
}