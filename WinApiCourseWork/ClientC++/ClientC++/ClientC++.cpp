#include "stdafx.h"
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>

using namespace std;

#pragma warning(disable: 4996)

const char* IPAdressServa1 = "127.0.0.1";
const char* IPAdressServa2 = "127.0.0.2";

const u_short PortServa1 = 1111;
const u_short PortServa2 = 1112;

bool ProvOnConnection1 = false;
bool ProvOnConnection2 = false;

SOCKET sockets[2];
HANDLE threads[2];


void CreateConnect(int numServ) {

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = numServ== 1 ? inet_addr(IPAdressServa1) : inet_addr(IPAdressServa2);
	addr.sin_port = numServ == 1 ? htons(PortServa1) : htons(PortServa2);
	addr.sin_family = AF_INET;

	SOCKET Connection = socket(AF_INET,SOCK_STREAM,NULL);
	if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {

		cout << "Не удалось подключиться к серверу.\n";
		return;
	}

	if (numServ == 1) { 
		sockets[0] = Connection;
		ProvOnConnection1 = true; 
	}
	else {
		sockets[1] = Connection;
		ProvOnConnection2 = true;
	}

	char newmessage[256];

	while (true) {

		if (recv(Connection, newmessage, sizeof(newmessage), NULL) == SOCKET_ERROR) {
		
			if (numServ == 1) closesocket(sockets[0]);
			else closesocket(sockets[1]);

			break;
		
		}
		else cout<<newmessage << endl;
	}
}


void ClientsMenu() {

	int result;
	cin >> result;

	switch (result)
	{
	case 1: {

		if (!ProvOnConnection1) {
			threads[0]=CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CreateConnect, (LPVOID)(1), NULL, NULL);
		}
		else {

			closesocket(sockets[0]);
			CloseHandle(threads[0]);
			threads[0] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CreateConnect, (LPVOID)(1), NULL, NULL);
		}

		ClientsMenu();
		break;
	}
	case 2: {

		if (!ProvOnConnection2) {
			threads[1]=CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CreateConnect, (LPVOID)(2), NULL, NULL);
		}
		else {

			closesocket(sockets[1]);
			CloseHandle(threads[1]);
			threads[1] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CreateConnect, (LPVOID)(2), NULL, NULL);
		}


		ClientsMenu();
		break;
	}
	default:
		ClientsMenu();
		break;
	}
}

int main(int argc, char* argv[]) {

	setlocale(LC_ALL, "Russian");

	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) exit(1);
	
	cout << "\nКурсовая работа\nДля подключения к серверам используйте цифры 1 и 2\n";
	ClientsMenu();

	system("pause");
	return 0;
}



