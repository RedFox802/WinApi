#include "stdafx.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

using namespace std;

HANDLE hEvent1, hEvent2, hEvent3;
HANDLE hPipe1, hPipe2;


//есть два отдельных файла для логов каждого сервера
//в потоках отдельно создаются отдельные каналы для каждого сервера
//каналы создаются и непрерывно читают сообщения если они есть и записывают в свои файлы.
//пути к файлам надо поменять на свои

// Запись входящего сообщения с помощью логгера
void WriteMessage(string message, string fileName) {

	std::ofstream out;
	out.open(fileName, ios::app);

	if (out.is_open())
	{

		out << message.c_str();
		out.close();
	}
	else {

		cout << "Не удается открыть файл лога";
	}
}

void CreateAndRead1()
{
	DWORD dwRead;

	while (true)
	{
		//создание канала
		hPipe1 = CreateNamedPipe(
			L"\\\\.\\pipe\\MyLogger1",
			PIPE_ACCESS_DUPLEX,//дввнаправленность
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE//данные записываются в канал как поток сообщений 
			| PIPE_WAIT,// Синхронное выполнение операций с каналом
			PIPE_UNLIMITED_INSTANCES,//количество экземпляров канала, ограничено только доступностью системных ресурсов
			0,// Количество байтов, которые нужно зарезервировать для выходного буфера
			0, //Количество байтов, которые нужно зарезервировать для выходного буфера
			NMPWAIT_USE_DEFAULT_WAIT,//Значение времени ожидания по умолчанию
			NULL// Без дополнительных атрибутов безопасности
		);

		if (hPipe1 != INVALID_HANDLE_VALUE)
		{
			break;
		}

	}

	SetEvent(hEvent1);
	cout << "Канал для первого сервера создан\n";


	while (true) {

		if (ConnectNamedPipe(hPipe1, NULL) != FALSE) break;
	}

	//чтение сообщений 
	while (true)
	{
		char buffer[1024];
		if (ReadFile(hPipe1, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
		{
			string text = buffer;
			WriteMessage(text, "C:\\Users\\xiaomi\\Desktop\\log1.txt");

		}
		else {
			ResetEvent(hEvent1);
			CreateAndRead1();
		}
	}

}

void CreateAndRead2()
{

	char buffer[1024];
	DWORD dwRead;

	while (true)
	{
		//создание канала
		hPipe2 = CreateNamedPipe(
			L"\\\\.\\pipe\\MyLogger2",
			PIPE_ACCESS_DUPLEX,//дввнаправленность
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE//данные записываются в канал как поток сообщений 
			| PIPE_WAIT,// Синхронное выполнение операций с каналом
			PIPE_UNLIMITED_INSTANCES,//количество экземпляров канала, ограничено только доступностью системных ресурсов
			0,// Количество байтов, которые нужно зарезервировать для выходного буфера
			0, //Количество байтов, которые нужно зарезервировать для выходного буфера
			NMPWAIT_USE_DEFAULT_WAIT,//Значение времени ожидания по умолчанию
			NULL// Без дополнительных атрибутов безопасности
		);

		if (hPipe2 != INVALID_HANDLE_VALUE)
		{
			break;
		}
	}

	SetEvent(hEvent2);
	cout << "Канал для второго сервера создан\n";

	while (true) {

		if (ConnectNamedPipe(hPipe2, NULL) != FALSE)break;

	}

	//чтение сообщений 
	while (true)
	{
		if (ReadFile(hPipe2, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
		{
			string text = buffer;
			WriteMessage(text, "C:\\Users\\xiaomi\\Desktop\\log2.txt");
		}
		else {
			ResetEvent(hEvent2);
			CreateAndRead2();
		}
	}

}


void CloseServer() {

	while (true) {

		int res;
		cin >> res;

		if (res == 1) {

			SetEvent(hEvent3);
			cout << "окончание работы" << endl;
			WriteMessage("Окончание работы сервера в связи с закрытием сервера логгирования\n", "C:\\Users\\xiaomi\\Desktop\\log2.txt");
			WriteMessage("Окончание работы сервера в связи с закрытием сервера логгирования\n", "C:\\Users\\xiaomi\\Desktop\\log1.txt");
			exit(0);

		}
	}
}

	
int main(int argc, char* argv[]) {

	//установка русского языка на консоли
	setlocale(LC_ALL, "Russian");

	//создется мьютекс, которым уже завладевают
	HANDLE hMutex = CreateMutex(NULL, TRUE, L"Mutex3");
	//если пытаются открыть несколько экземпляров сервера, то они не работают и закрываются
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		exit(0);
	}

	//события о создании каналов
	hEvent1 = CreateEvent(NULL, TRUE, FALSE, L"Working1");
	hEvent2 = CreateEvent(NULL, TRUE, FALSE, L"Working2");
	hEvent3 = CreateEvent(NULL, TRUE, FALSE, L"CloseEwent");
	
	cout << "Сервер предназначенный для логгирования\n";
	cout << "Будут созданы каналы для передачи сообщенйи с серверов\n";

	//для каждого канала отдельный поток 
	std::thread t1(CreateAndRead1);
	std::thread t2(CreateAndRead2);
	cout << "Для окончания работы нажмите 1\n";
	std::thread t3(CloseServer);
	t1.join();
	t2.join();
	t3.join();

	system("pause");
	return 0;
}