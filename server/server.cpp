#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include<algorithm>
#include <stdlib.h>
using namespace std;
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)
SYSTEMTIME t;
const int size_of_clients = 1000;
const int block_word_size = 31;
string log_file = "logs.txt";
string Server_Name = "Server";
string names[size_of_clients];
string blocked_words[block_word_size] = {
	"nigger", "nigga", "naga", "ниггер", "нига", //5
	"нага", "faggot", "пидор", "пидорас", "педик", //5
	"гомик", "хохол", "хач", "жид", "virgin",//5
	"хиджаб", "даун", "аутист", "дебил", "retard",//5
	"simp", "incel", "девственник", "cимп", "инцел",//5
	"cunt", "пизда", "куколд", "белый", "натурал",//5
	"гетеросексуал"//1
};
const char Local_IP[16] = "192.168.0.106";
const char LOCAL_PORT[4] = "140";
int Counter = 0;
int Active = 0;
ofstream Logs;
SOCKET Connections[size_of_clients];
WSAData wsaData;
ADDRINFO hints;
ADDRINFO* addrrez = NULL;
WORD DLLVersion = MAKEWORD(2, 2);
SOCKET ClientSock = INVALID_SOCKET;
SOCKET sListen = INVALID_SOCKET;
void Print_logs(string line) {//вывод в консоль и тоже самое в файл логов
	GetLocalTime(&t);
	cout << "[" + to_string(t.wYear) + "." + to_string(t.wMonth) + "." + to_string(t.wDay) + "_" + to_string(t.wHour) + ":" + to_string(t.wMinute) + ":" + to_string(t.wSecond) + "] " + line << endl;
	Logs.open(log_file, ios::app);//открытие файла логов
	Logs << "[" + to_string(t.wYear) + "." + to_string(t.wMonth) + "." + to_string(t.wDay) + "_" + to_string(t.wHour) + ":" + to_string(t.wMinute) + ":" + to_string(t.wSecond) + "] " + line << endl;
	Logs.close();//закрытие файла логов
}//функция проверки на ошибки 
int Closing(int s_type = 0, int function = 0,int index = 0, SOCKET A = INVALID_SOCKET)
{ 
	switch (s_type)
	{
	case(1):// start_up
		if (function != 0)
		{
			cout << "startup err: " << function << endl;
			return 1;
		}
		return 0;
	case(2)://getaddrinfo err
		if (function != 0)
		{
			cout << "getaddrinfo err: " << function << endl;
			freeaddrinfo(addrrez);
			WSACleanup();
			return 1;
		}
		return 0;
	case(3)://recv err
		if (function == 0)
		{
			Print_logs("Connection closing for client #" + to_string(index) + ".\n\t\t\t\tClosing connection with " + names[index] + "...");
			closesocket(A == INVALID_SOCKET ? Connections[index] : A);
			return 0;
		}
		else if (function < 0)
		{
			Print_logs("Recv ERROR.\n\t\t\t\tClosing connection...");
			closesocket(A == INVALID_SOCKET ? Connections[index] : A);
			return 0;
		}
		return 1;
	case(4)://send err
		if (function == SOCKET_ERROR)
		{
			Print_logs("Error to send data");
			char rets = A != INVALID_SOCKET ? 2 : 0;
			closesocket(A == INVALID_SOCKET ? Connections[index] : A);
			WSACleanup();
			return rets;
		}
		return 1;
	case(5):// Not CONNECTED WITH SERVER
		if (function == SOCKET_ERROR)
		{
			cout << "Unable connect to server..." << endl;
			closesocket(A == INVALID_SOCKET ? Connections[index] : A);
			Connections[index] = INVALID_SOCKET;
			freeaddrinfo(addrrez);
			WSACleanup();
			return 1;
		}
		return 0;
	default:
		break;
	}
}//отправка сообщения
bool send_msg(string line, int index, SOCKET A = INVALID_SOCKET)
{
	int msg_size = line.size();
	// отправка размера сообщения
	int rety = Closing(4, send(A==INVALID_SOCKET?Connections[index]:A, (char*)&msg_size, sizeof(int), 0), A == INVALID_SOCKET ? Connections[index] : A);
	if (!rety)
		return 0;
	// отправка сообщения
	rety = Closing(4, send(A == INVALID_SOCKET ? Connections[index] : A, line.c_str(), (int)msg_size, 0), A == INVALID_SOCKET ? Connections[index] : A);
	if (!rety)
		return 0;
	return 1;
	//в случае удачи возвращает 1
}//получение сообщений от клиента с адресом index
string recv_msg(int index) 
{
	int msg_size;
	if (!Closing(3, recv(Connections[index], (char*)&msg_size, sizeof(int), NULL), index))
		return "";
	char* msg = new char[msg_size + 1];
	msg[msg_size] = '\0';
	if (!Closing(3, recv(Connections[index], msg, msg_size, NULL), index))
		return "";
	return msg;
}// функция работы с клиентами запускаемая в отдельном потоке при каждом новом подключении
void ClientHandler(int index) 
{
	bool rety = 1;	
	bool banned = 0;
	// переменные флаги для управления циклами
	string msgs = recv_msg(index);
	msgs.shrink_to_fit();
	if (msgs.empty())
	{
		names[index] = "";
		Active--;
		return;
	}
	string index_name = msgs;
	//получение имени
	for (size_t i = 0; i < msgs.size(); i++)
		msgs[i] == ' ' ? msgs[i] = '_' : 1;
	names[index] = msgs;
	// запись имени с "_" вместо пробелов в массив имен
	Print_logs("Connected client: " + index_name);
	// логи
	msgs.clear();
	while (rety) {//основной цикл обработки данных
		msgs = recv_msg(index);
		if (msgs.empty())
			break;
		//ожидание получения сообщения
		string local_name = "";
		string msgg="";
		//создание переменных миени отправителя и отправляемого сообщения
		string test = msgs;
		transform(test.begin(), test.end(), test.begin(), ::tolower);
		for (size_t i = 0; i < block_word_size; i++)
			(string::npos!=test.find(blocked_words[i]))?banned = 1:1;
		//проверка принятого сообщения на наличие запрещенных слов
		if (Active == 1) {
			local_name = Server_Name;
			Print_logs(to_string(msgs.size()) + " " + index_name + " " + msgs);
			msgs = "You one in room.";
		}// проверка на наличие собеседников
		if (banned)
		{
			local_name = Server_Name;
			Print_logs(to_string(msgs.size()) + " " + index_name + " " + msgs);
			msgs = "You has been banned";
		}// отправка в бан за нарушение правил чата
		if (msgs[0] == '@' and local_name == "")
		{
			//установка сообщения только пользователю, указанному в сообщении
			local_name = msgs.substr(1, msgs.find(':') - 1);
			msgg = index_name + " -> " + local_name + ": " + msgs.substr(msgs.find(':') + 1);
			for (size_t i = 0; i < local_name.size(); i++)
				local_name[i] == ' ' ? local_name[i] = '_' : 1;
		}
		else
		{
			//установка сообщения всем или вернуть обратно системное сообщение
			msgg = ((local_name == "") ? index_name : local_name) + ": " + msgs;
		}
		Print_logs(to_string(msgg.size()) + " " + index_name + " " + msgg + " ");
		//логирование данных перед отправкой
		if (local_name == "" and Active > 1)
		{
			for (size_t i = 0; i < Counter; i++)
			{
				if (names[index] == names[i] or names[i].empty()) 
					continue;
				send_msg(msgg, i);
			}
		}//кроме отправителя и свободных ячеек
		else
		{//отправителю
			if (local_name == Server_Name)
			{
				send_msg(msgg, index);
			}
			else
			{
				bool Correct_name = 1;
				int adres_reciver;
				for (size_t i = 0; i < Counter; i++)
				{
					if (local_name != names[i])
						continue;
					adres_reciver = i;
					break;
				}
				if (adres_reciver < 0 or adres_reciver > Counter)
				{
					Print_logs("Uncorrect Name");
					msgg = Server_Name + ": Uncorrect Name";
					Correct_name = 0;
				}
				//проверка корректности введенного имени в сообщении
				if (Correct_name)
					send_msg(msgg, adres_reciver);
				else
					send_msg(msgg, index);
			}
		}
		banned ? rety = 0 : 1;
		//если в бане то не продолжаем цикл
	}
	names[index] = "";
	Active--;
	//очистка имени и уменьшение количествка активных соединений
}


int main(int argc, char* argv[]) 
{
	string ip = Local_IP;
	ip += ':';
	ip += LOCAL_PORT;
	Print_logs("\n\n-----Starting the " + Server_Name + "-----");
	setlocale(LC_ALL, "");
	system("chcp 1251");
	//старт запуска
	int rez;
	if (Closing(1, WSAStartup(DLLVersion, &wsaData)))
		return 1;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;//tcp ip
	hints.ai_flags = AI_PASSIVE;
	if (Closing(2, getaddrinfo(Local_IP, LOCAL_PORT, &hints, &addrrez)))
		return 1;
	sListen = socket(addrrez->ai_family, addrrez->ai_socktype, addrrez->ai_protocol);
	if (sListen == INVALID_SOCKET)
	{
		Print_logs("Ssocket create err");
		
		Print_logs("IP adress: " + ip);
		freeaddrinfo(addrrez);
		WSACleanup();
		return 1;
	}
	rez = bind(sListen, addrrez->ai_addr, (int)addrrez->ai_addrlen);
	if (rez == SOCKET_ERROR)
	{
		Print_logs("Binding socket fail");
		Print_logs("IP adress: " + ip);
		closesocket(sListen);
		sListen = INVALID_SOCKET;
		freeaddrinfo(addrrez);
		WSACleanup();
		return 1;
	}
	//создание сокета
	rez = listen(sListen, SOMAXCONN);
	if (rez == SOCKET_ERROR)
	{
		Print_logs("Listen error");
		closesocket(sListen);
		freeaddrinfo(addrrez);
		WSACleanup();
		return 1;
	}
	//настройка на прослушивание порта
	for (int i = 0; i < Counter + 1; i++) 
	{
		ClientSock = accept(sListen, NULL, NULL);
		if (ClientSock == INVALID_SOCKET)//~0 
		{
			Print_logs("Accepting socket error");
			closesocket(sListen);
			freeaddrinfo(addrrez);
			WSACleanup();
			return 1;
		}
		//подключение клиента
		Print_logs("Client Connected!");
		string msg = "Enter your name:";
		int msg_size = msg.size();
		if (send_msg(msg, 0, ClientSock)) {
			Connections[i] = ClientSock;
			Counter++;
			Active++;
			CreateThread(
				NULL,
				NULL,
				(LPTHREAD_START_ROUTINE)ClientHandler,
				(LPVOID)(i),
				NULL,
				NULL
			);
		}
		//запрос имени и перенос в новый поток а функция точки входа остается прослушивать порт для поиска новых подключений
	}
	Print_logs("----" + Server_Name + " Closed----");
	closesocket(ClientSock);
	return 0;
}
