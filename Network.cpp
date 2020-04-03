#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Network.h"
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define CERR_NOT_ENABLED "Socket not enabled. Doing nothing on function call: " << __FUNCTION__ << endl;
#define CERR_ON_FUNC(errname) errname << "on function call: " << __FUNCTION__ << endl;

static int port = 50000;
static bool socketEnabled = false;
static SOCKET client = INVALID_SOCKET;
static SOCKET server = INVALID_SOCKET;

using std::cerr;
using std::cout;
using std::endl;

void Network::useSettings(SettingsWrapper & sw) {
	port = sw.socket_port;
	socketEnabled = sw.socket_enable;
}

bool Network::startServer() {
	if (socketEnabled) {
		WSADATA wsaData;
		int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (err) {
			cerr << "WSAStartup failed: " << err << endl;
			return false;
		}

		struct addrinfo *result = NULL, *ptr = NULL, hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		{
			err = getaddrinfo(NULL, std::to_string(port).data(), &hints, &result);
			if (err) {
				WSACleanup();
				cerr << "getaddrinfo failed: " << err << endl;
				return false;
			}
		}

		server = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

		if (server == INVALID_SOCKET) {
			cerr << "socket failed: " << WSAGetLastError() << endl;
			freeaddrinfo(result);
			WSACleanup();
			return false;
		}

		err = bind(server, result->ai_addr, (int)result->ai_addrlen);
		if (err == SOCKET_ERROR) {
			cerr << "bind failed: " << WSAGetLastError() << endl;
			server = INVALID_SOCKET;
			closesocket(server);
			server = INVALID_SOCKET;
			freeaddrinfo(result);
			WSACleanup();
			return false;
		}

		freeaddrinfo(result);

		if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
			cerr << "listen failed: " << WSAGetLastError() << endl;
			closesocket(server);
			server = INVALID_SOCKET;
			WSACleanup();
			return false;
		}

		return true;
	}
	else {
		cerr << CERR_NOT_ENABLED;
		return false;
	}
}

bool Network::acceptConnection() {
	if (!socketEnabled) {
		cerr << CERR_NOT_ENABLED;
		return false;
	}
	if (server == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Server");
		return false;
	}

	client = accept(server, NULL, NULL);

	if (client == INVALID_SOCKET) {
		cerr << "Failed to accept connection." << endl;
		return false;
	}

	cout << "Connection successful" << endl;
	return true;
}

void Network::sendData(char * data, int len) {
	if (!socketEnabled) {
		cerr << CERR_NOT_ENABLED;
		return;
	}
	if (server == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Server INVALID_SOCKET");
		return;
	}
	if (client == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Client INVALID_SOCKET");
		return;
	}

	int err = send(client, data, len, 0);
	if (err == SOCKET_ERROR) {
		cerr << "Sending data failed: " << WSAGetLastError() << endl;
		return;
	}
}

std::vector<char> Network::recvData() {
	if (!socketEnabled) {
		cerr << CERR_NOT_ENABLED;
		return std::vector<char>();
	}
	if (server == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Server INVALID_SOCKET");
		return std::vector<char>();
	}
	if (client == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Client INVALID_SOCKET");
		return std::vector<char>();
	}

	u_long nBytes;
	int err = ioctlsocket(client, FIONREAD, &nBytes);
	if (err == SOCKET_ERROR) {
		cerr << "Finding number of bytes to read failed: " << WSAGetLastError() << endl;
		return std::vector<char>();
	}

	std::vector<char> returner(nBytes);
	int nBytesActual = recv(client, returner.data(), nBytes, 0);
	if (nBytesActual == SOCKET_ERROR) {
		cerr << "Error reading bytes from socket: " << WSAGetLastError() << endl;
		return std::vector<char>();
	}

	if ((int64_t) nBytes > (int64_t) nBytesActual) {
		cout << "Received more bytes durring read." << endl;
	}
	return returner;
}

int Network::recvData(char * data, int len) {
	if (!socketEnabled) {
		cerr << CERR_NOT_ENABLED;
		return 0;
	}
	if (server == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Server INVALID_SOCKET");
		return 0;
	}
	if (client == INVALID_SOCKET) {
		cerr << CERR_ON_FUNC("Server INVALID_SOCKET");
		return 0;
	}

	int nBytesActual = recv(client, data, len, 0);

	if (nBytesActual == SOCKET_ERROR) {
		cerr << "Error reading bytes from socket: " << WSAGetLastError() << endl;
	}

	return nBytesActual;

}

int Network::getBytesReady()
{
	u_long nBytes;
	int err = ioctlsocket(client, FIONREAD, &nBytes);
	if (err == SOCKET_ERROR) {
		cerr << "Finding number of bytes to read failed: " << WSAGetLastError() << endl;
	}
	return nBytes;
}
