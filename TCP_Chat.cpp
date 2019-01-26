#include <cstdlib>
#include <conio.h>
#include <iostream>
#include <winsock2.h> 
#include <windows.h> //Podkluchat' tol'ko posle winsock2!!!! 
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <iomanip>

#pragma comment (lib, "ws2_32.lib")

#define SERVERADDR_ "127.0.0.1"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

using namespace std;

enum ConsoleColor
{
	Black = 0,
	Blue = 1,
	Green = 2,
	Cyan = 3,
	Red = 4,
	Magenta = 5,
	Brown = 6,
	LightGray = 7,
	DarkGray = 8,
	LightBlue = 9,
	LightGreen = 10,
	LightCyan = 11,
	LightRed = 12,
	LightMagenta = 13,
	Yellow = 14,
	White = 15
};

COORD ChatPos; //Место вывода очередного сообщения в чате
HANDLE ConsoleHandle;
SOCKET ConnectSocket = INVALID_SOCKET;
bool Shutdown = false;

const COORD MsgBoxPos = { 1, 0 }; //Координаты поля ввода
const COORD BaseChatPos = { 0, 2 }; //Начальные координаты выводимых в чате сообщений

const ConsoleColor YourColor = LightBlue; //Цвет Ваших сообщений в чате
const ConsoleColor PartnerColor = LightMagenta; //Цвет других сообщений в чате
const ConsoleColor BackgroundColor = White; //Цвет фона сообщений

const int ADDRSize = 100; //константа максимального размера адреса
const int NicknameSize = 30; //константа максимального размера псевдонима

void SetColor(ConsoleColor text, ConsoleColor background)
{
	SetConsoleTextAttribute(ConsoleHandle, (WORD)((background << 4) | text));
}

int InputFromServer()
{
	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	COORD CurrChatPos;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int iResult;

	cout << "Second thread started!\n";
	cout << "Opening chat...\n";
	Sleep(1900);

	while (!Shutdown)
	{
		Sleep(10);

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0); //Получаем данные от сервера

		if (iResult <= 0 || stricmp("#done", recvbuf) == 0) //Если ошибка приема или мы закрыли соединение
		{
			Shutdown = true;
			break;
		}

		GetConsoleScreenBufferInfo(ConsoleHandle, &ConsoleInfo); //Запоминаем позицию курсора в консоли
		
		SetConsoleCursorPosition(ConsoleHandle, ChatPos); //Устанавливаем курсор на окно чата

		SetColor(PartnerColor, BackgroundColor); //Выводим сообщение
		cout << recvbuf;
		SetColor(Black, White);

		ChatPos.Y += (int)(iResult / 80) + 1; //Расчет расстояния, на которое нужно перенести курсор в консоли в зависимости от длины сообщения

		SetConsoleCursorPosition(ConsoleHandle, ConsoleInfo.dwCursorPosition); //Вовзвращаем курсор на прежнее место

		ZeroMemory(&recvbuf, recvbuflen);
	}
	SetConsoleCursorPosition(ConsoleHandle, MsgBoxPos);
	cout << "\r>Server disconnected. Please restart client.\n";

	closesocket(ConnectSocket);
	Shutdown = true; //На всякий случай
	return 0;
}

void PrintHeader()
{
	srand(time(NULL));
	SetColor(static_cast<ConsoleColor>(rand() % 15), White); //static_cast переводит значение из int в enum
	cout << R"(
                _____ _____ ______   _____  _   _   ___ _____
               |_   _/  __ \| ___ \ /  __ \| | | | / _ \_   _|
                 | | | /  \/| |_/ / | /  \/| |_| |/ /_\ \| |  
                 | | | |    |  __/  | |    |  _  ||  _  || |  
                 | | | \__/\| |     | \__/\| | | || | | || |  
                 \_/  \____/\_|      \____/\_| |_/\_| |_/\_/  
)" << endl << endl;
	SetColor(Black, White);
}

int main()
{
	char SERVERADDR[ADDRSize];
	ZeroMemory(&SERVERADDR, ADDRSize);

	system("color F0"); //Перекрашиваем консоль
	ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	ChatPos = BaseChatPos;

	//PlaySound(TEXT("music.wav"), NULL, SND_FILENAME | SND_ASYNC); //Хочешь немного музыки?))

	PrintHeader(); //Выводим красивую надпись

	cout << "Enter your nickname: ";
	char Nickname[NicknameSize];
	cin >> Nickname;

	cout << "Enter server address: ";
	string ServAddr;
	cin >> ServAddr;
	if (ServAddr == "1") memcpy(SERVERADDR, "127.0.0.1", sizeof("127.0.0.1"));  //Для быстрого ввода. Вводишь 1, автоматически вставляется 127.0.0.1
	else memcpy(SERVERADDR, ServAddr.c_str(), ServAddr.length());

	// Шаг 1 - подключение библиотеки версии 2.2
	int iResult;
	WSADATA wsaData;

	// Макрос MAKEWORD(lowbyte, highbyte) объявлен в Windef.h
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup error: %d\n", WSAGetLastError());
		_getch();
		return -1;
	}
	// Выясняем, а поддерживается ли версия библиотеки 2.2
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		_getch();
		return -1;
	}
	printf("The Winsock 2.2 dll was found ok\n");

	// Шаг 2 - открытие сокета

	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(SERVERADDR, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		_getch();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		_getch();
		return 1;
	}

	cout << "Successfully connected to " << SERVERADDR << endl;

	iResult = send(ConnectSocket, Nickname, sizeof(Nickname), 0); //Отправляем ник на север
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		_getch();
		return 1;
	}

	char codebuf[2];

	iResult = recv(ConnectSocket, codebuf, sizeof(codebuf), 0); //Принимаем ответ от севера

	if (!stricmp(codebuf, "1")) //В случае, если сервер ответил "1", значит нет места для новых клиентов
	{
		cout << "Not enough space for new clients. Please try again later.\n";
		system("pause");
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	cout << "Successfully logged in.\n";

	//Создаем второй поток для приема сообщений
	cout << "Creating second thread...\n";
	thread InputFromServerThread(InputFromServer);

	Sleep(2000); //Ждем немного

	system("cls");
	printf(">");

	cin.clear(); //Очищаем буфер ввода
	fflush(stdin);

	//Процедура отправки сообщений
	string msg;
	while (1)
	{
		getline(cin, msg); // чтение сообщения с клавиатуры
		if (Shutdown) //В случае, если был запрошен выход
		{
			InputFromServerThread.join();
			WSACleanup();
			return 0;
		}

		SetConsoleCursorPosition(ConsoleHandle, MsgBoxPos); //Откатили курсор на начало поля ввода
		if (msg == "") continue; //Если Вы ввели ничего, то возвращаемся на ожидание ввода

		for (int i = 0; i < 79 + 80; i++) printf(" "); //Очистили строку ввода и строку ниже нее
		SetConsoleCursorPosition(ConsoleHandle, MsgBoxPos); //Снова откатили курсор на начало поля ввода

		if (msg[0] == '/') //Если была введена команда
		{
			if (msg == "/clear")
			{
				ChatPos = BaseChatPos;
				system("cls");
				printf(">");
				continue;
			}
			if (msg == "/exit")
			{
				char exitmsg[] = "#exit";
				iResult = send(ConnectSocket, exitmsg, sizeof(exitmsg), 0); //Отправляем сообщение о закрытии подключения

				Shutdown = true;

				iResult = shutdown(ConnectSocket, SD_SEND); //Закрываем поток отправки
				if (iResult == SOCKET_ERROR) {
					printf("shutdown failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					_getch();
					return 1;
				}
				
				InputFromServerThread.join(); //Ждем выключения второго потока
				
				WSACleanup();
				cout << ">...Exit. Press any key to close window...<";
				_getch();
				return 0;
			}
			printf("\n<!No such command!>");
			_getch();
			printf("\r");
			for (int i = 0; i < 80; i++) printf(" "); //Очищаем надпись <!No such command!>
			SetConsoleCursorPosition(ConsoleHandle, MsgBoxPos); //Возвращаемся на поле ввода
			continue;
		}

		SetConsoleCursorPosition(ConsoleHandle, ChatPos);

		SetColor(YourColor, BackgroundColor);
		cout << "You: " << msg;
		SetColor(Black, White);

		ChatPos.Y += (int)((5 + msg.length()) / 80) + 1; //Расчет расстояния, на которое нужно перенести курсор в консоли в зависимости от длины сообщения

		SetConsoleCursorPosition(ConsoleHandle, MsgBoxPos);

		// Передача сообщений на север
		iResult = send(ConnectSocket, msg.c_str(), msg.length() + 1, 0);
	}

	return 0;
}