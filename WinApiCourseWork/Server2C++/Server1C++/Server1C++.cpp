#include "stdafx.h"
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
using namespace std;

#pragma warning(disable: 4996)

const int MaxConnection = 20;
SOCKET ArrayConnection[MaxConnection];

string ArrayPhysProcents[MaxConnection];
string ArrayVirtualProcents[MaxConnection];

HANDLE CloseEvent;

HANDLE hPipe;//канал для передачи логов


// Функция получения текущего времени для лога
string GetSystemForLog() {

	char buffer[80];
	time_t seconds = time(NULL);
	tm* timeinfo = localtime(&seconds);
	const char* format = "%H:%M:%S";
	strftime(buffer, 80, format, timeinfo);

	string time = (buffer);
	return "[" + time + "]";
}


//отправка сообщений с логами
void WriteInLog(string messageLog) {

	char buffer[256];
	DWORD nBytesRead;

	strcpy_s(buffer, messageLog.c_str());
	//запись в канал, который уже открыт
	WriteFile(hPipe, &buffer, sizeof(buffer), &nBytesRead, NULL);
}

void MessageForConsoleServer() {

	auto timeNow = chrono::system_clock::now();
	time_t timeSentToClientMessage = chrono::system_clock::to_time_t(timeNow);
	cout << "Новое сообщение отправлено клиенту " << ctime(&timeSentToClientMessage) << "\n";
}

string CreateStringInformation(bool isPrys) {

	MEMORYSTATUSEX Status;
	Status.dwLength = sizeof(Status);
	GlobalMemoryStatusEx(&Status);

	if (isPrys) {

		return "Физической памяти используется " + to_string(Status.dwMemoryLoad) + " процентов \n";
	}
	else {

		int TotalVirtual = Status.ullTotalVirtual/1024;
		int AvailVirtual = Status.ullAvailVirtual/1024;

		int OccupiedVirtual = TotalVirtual - AvailVirtual;
		double a = TotalVirtual / 100;
		double b = OccupiedVirtual / a;

		char buff[256];
		sprintf(buff, "Виртуальной памяти используется %.2f процентов \n", b);
		return buff;
	}
}


void SentMessages(int index) {

	ArrayPhysProcents[index] = CreateStringInformation(true);
	ArrayVirtualProcents[index] = CreateStringInformation(false);

	string result = ArrayPhysProcents[index] + ArrayVirtualProcents[index];

	char messageArray[256];
	strcpy(messageArray, result.c_str());
	send(ArrayConnection[index], messageArray, sizeof(messageArray), NULL);

	WriteInLog(GetSystemForLog() + "Отправлено следующее сообщение клиенту " + to_string(index + 1) + " :\n" + messageArray);
	MessageForConsoleServer();

	while (true) {

		Sleep(15000);

		string newPhys = CreateStringInformation(true);
		string newVirtual = CreateStringInformation(false);

		if (ArrayPhysProcents[index] != newPhys) {

			ArrayPhysProcents[index] = newPhys;

			string newRes = "Данные обновлены: \n" + ArrayPhysProcents[index];
			strcpy(messageArray, newRes.c_str());

			if (send(ArrayConnection[index], messageArray, sizeof(messageArray), NULL) != SOCKET_ERROR) {
				MessageForConsoleServer();
				WriteInLog(GetSystemForLog() + "Отправлено следующее сообщение клиенту " + to_string(index + 1) + " :\n" + messageArray);
			}
			else { 
				WriteInLog(GetSystemForLog() + "Соединение с клиентом " + to_string(index + 1) + " прервано\n");
				break; 
			}

		}

		if (ArrayVirtualProcents[index] != newVirtual) {

			ArrayVirtualProcents[index] = newVirtual;

			string newRes = "Данные обновлены: \n" + ArrayVirtualProcents[index];
			strcpy(messageArray, newRes.c_str());

			if (send(ArrayConnection[index], messageArray, sizeof(messageArray), NULL) != SOCKET_ERROR) {
				MessageForConsoleServer();
				WriteInLog(GetSystemForLog() + "Отправлено следующее сообщение клиенту " + to_string(index + 1) + " :\n" + messageArray);
			}
			else {

				WriteInLog(GetSystemForLog() + "Соединение с клиентом " + to_string(index + 1) + " прервано\n");
				break;
			}
		}
	}
}

void CloseServer(int a) {

	while (true) {

		int command;
		cin >> command;

		if (command == 1) {

			WriteInLog(GetSystemForLog() + "Сервер завершил работу по требованию пользователя\n");
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
			exit(0);
		}
	}
}

void CloseServer2(int k) {

	WaitForSingleObject(CloseEvent, INFINITE);
	cout << "Завершение работы в связи с отключением сервера логов";
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	exit(0);
}


int main(int argc, char* argv[]) {

	HANDLE hMutex = CreateMutex(NULL, TRUE, L"Mutex2");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		exit(0);
	}

	setlocale(LC_ALL, "Russian");


	HANDLE MyEvent = CreateEvent(NULL, TRUE, FALSE, L"Working2");
	CloseEvent = CreateEvent(NULL, TRUE, FALSE, L"CloseEwent");

	cout << "Ожидвется запуск серверера логгирования\n";
	WaitForSingleObject(MyEvent, INFINITE);
	cout << "Начало работы\n";

	//открываем канал созданный на сервере, в цикле потому что с первого раза зараза не всегда создается корректно
	while (true) {

		hPipe = CreateFile(L"\\\\.\\pipe\\MyLogger2",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hPipe != INVALID_HANDLE_VALUE) break;
	}


	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) exit(1);

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.2");
	addr.sin_port = htons(1112);
	addr.sin_family = AF_INET;

	SOCKET sListen = socket(AF_INET,SOCK_STREAM,NULL);
	bind(sListen,(SOCKADDR*)&addr,sizeof(addr));
	listen(sListen, MaxConnection);

	cout << "Ожидание подключений\n";
	WriteInLog(GetSystemForLog() + "Сервер запущен и ожидает подключений!\n");
	cout << "Для окончания работы нажмите 1\n";
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CloseServer, (LPVOID)(1), NULL, NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CloseServer2, (LPVOID)(1), NULL, NULL);

	SOCKET newConnection;
	for (int i = 0; i < MaxConnection; i++) {

		newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
		if (newConnection != 0) {

			cout << "Новое подключение\n";
			WriteInLog(GetSystemForLog() + "Новое подключение\n");
			ArrayConnection[i] = newConnection;
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SentMessages, (LPVOID)(i), NULL, NULL);
		}
	}

	system("pause");
	return 0;
}