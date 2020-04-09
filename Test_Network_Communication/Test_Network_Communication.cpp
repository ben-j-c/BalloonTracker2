#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#include <SettingsWrapper.h>

#pragma comment(lib, "Ws2_32.lib")

using std::cerr;
using std::cout;
using std::endl;

double sendArray[] = {1.0, 2.0, 0.0, 3.14, 365.25, NAN, INFINITY, -INFINITY};

int main() {
	int buffLen = sizeof(sendArray);
	char* sendData = (char*) sendArray;
	SettingsWrapper sw("../settings.json");
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err) {
		cerr << "WSAStartup failed: " << err << endl;
		exit(EXIT_FAILURE);
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	{
		int socket = 50000;
		if (sw.socket_enable)
			socket = sw.socket_port;
		err = getaddrinfo(NULL, std::to_string(socket).data() , &hints, &result);
		if (err) {
			WSACleanup();
			cerr << "getaddrinfo failed: " << err << endl;
			exit(EXIT_FAILURE);
		}
	}

	SOCKET sock = INVALID_SOCKET;
	sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (sock == INVALID_SOCKET) {
		cerr << "socket failed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	err = bind(sock, result->ai_addr, (int)result->ai_addrlen);
	if (err == SOCKET_ERROR) {
		cerr << "bind failed: " << WSAGetLastError() << endl;
		closesocket(sock);
		freeaddrinfo(result);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed: " << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	int cont;
	do {
		cout << "Waiting for connection..." << endl;
		SOCKET cSock = INVALID_SOCKET;
		cSock = accept(sock, NULL, NULL);
		if (cSock != INVALID_SOCKET) {
			cout << "Connection established. Sending data." << endl;
			err = send(cSock, sendData, buffLen, 0);
			if (err == SOCKET_ERROR) {
				cerr << "send failed: " << WSAGetLastError() << endl;
			}

			closesocket(cSock);
		}
		else {
			cerr << "Connection failed" << endl;
		}
		cout << "Allow another connection? (0 for quit)" << endl;
		std::cin >> cont;
	} while (cont);
	closesocket(sock);
	WSACleanup();
}