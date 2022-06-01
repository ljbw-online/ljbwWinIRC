#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h> // Also necessary for Windows sockets
#include <stdio.h>
#include <string> // for getline

// "... indicates to the linker that the Ws2_32.lib file is needed"
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "6667"
#define DEFAULT_BUFLEN 512

int sendIRC(SOCKET sock, std::string buffer) {
	return send(sock, buffer.c_str(), buffer.size(), 0);
}

int main() {
	WSADATA wsaData;

	int iResult;

	// Initialise Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	
	SOCKET ListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	// result->ai_addrlen is converted to an int because it's actually a "size_t"

	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Free the memory allocated by the getaddrinfo function for the 
	// address information
	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);

	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	char recvbuf[DEFAULT_BUFLEN];
	iResult = 0;
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	std::string sendstring;

	// Receive until the peer shuts down the connection
	do {
		// Set to zeros to prevent garbage characters being present in recvstring
		memset(&recvbuf, '\0', sizeof(recvbuf));

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			//printf("Bytes received: %d\n", iResult);

			std::string recvstring = std::string(recvbuf);
			// recvstring.length() == iResult

			std::string::size_type pos = std::string::npos;
			std::string::size_type len = recvstring.length();
			do {
			    pos = recvstring.find("\n", 0);

			    std::string line = recvstring.substr(0, pos);

				recvstring = recvstring.substr(pos + 1, std::string::npos);
				len = recvstring.length();

				if (line.find("PASS", 0) != 0) {
					std::cout << line << "\n";
				}
				else {
					std::cout << "PASS ***********\n";
				}
				//std::cout << "---\n";
			} while (pos != std::string::npos);
		}
		else if (iResult == 0) {
			//printf("Connection closing...\n");

			getline(std::cin, sendstring); // Necessary for strings containing spaces
			iSendResult = sendIRC(ClientSocket, sendstring);

			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (sendstring != "quit");

	// Shutdown the send half of the connection since no more data will be sent
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	closesocket(ClientSocket);
	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}