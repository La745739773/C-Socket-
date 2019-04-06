#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<string>
using namespace std;
#pragma comment(lib,"ws2_32.lib") //windows socket2 32��lib��

//һ��Ҫ��֤����˺Ϳͻ��ˣ�����ϵͳ���� ���ݽṹ�ֽ�˳��ʹ�С��֤һ�� 
struct DataPackage
{
	int age;
	char name[32];
};

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGINOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
//��Ϣͷ
struct DataHeader
{
	short dataLength;    //���ݳ��� 32767�ֽ�
	short cmd;
};

struct Login : public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char Password[32];
};
struct Logout :public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGINOUT;
	}
	char userName[32];
};
struct LoginResult :public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};
struct LogoutResult :public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

int main()
{
	//���� windows socket 2.x ����
	WORD versionCode = MAKEWORD(2, 2);	//����һ���汾�� 
	WSADATA data;
	WSAStartup(versionCode, &data);  //����Socket����API�ĺ���

	//1. ����һ��Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0); //�����������ӵ�ipv4��socket

	if (INVALID_SOCKET == _sock)	//SIocket ÿһ���������ж��Ƿ�ɹ�����
	{
		cout << "ERROR: SOCKET ����ʧ��" << endl;
	}
	else
	{
		cout << "SUCCESS: SOCKET �����ɹ�" << endl;
	}

	//2. ���ӷ����� connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567); //host to net short
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))	//SIocket ÿһ���������ж��Ƿ�ɹ�����
	{
		cout << "ERROR: SOCKET ����ʧ��" << endl;
		getchar();
		return 0;
	}
	else
	{
		cout << "SUCCESS: SOCKET ���ӳɹ�" << endl;
	}

	while (true)
	{
		// 3 ������������
		char cmdBuf[128] = {};
		cout << "��������: ";
		cin >> cmdBuf;
		// 4 ��������
		if (strcmp(cmdBuf, "exit") == 0)
		{
			break;
		}
		else if (0 == strcmp(cmdBuf,"login"))
		{
			Login _login;
			strcpy(_login.userName, "Evila");
			strcpy(_login.Password, "Evila_Password");
			// 5 �������������������
			send(_sock, (const char*)&_login, sizeof(Login), 0);

			//6. ���ܷ�������Ϣ recv
			LoginResult _lgRes;
			recv(_sock, (char*)&_lgRes, sizeof(LoginResult), 0);
			cout<<"LoginResult: " << _lgRes.result << endl;
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout _logout;
			strcpy(_logout.userName, "Evila");
			// 5 ������������������� 
			send(_sock, (const char*)&_logout, sizeof(Logout), 0);

			//6. ���ܷ�������Ϣ recv
			LogoutResult _lgRes;
			//��������
			recv(_sock, (char*)&_lgRes, sizeof(LogoutResult), 0);
			cout << "LogoutResult: " << _lgRes.result << endl;
		}
		else
		{
			cout << "��֧�ֵ�������������롣" << endl;
		}
	}
	//7. �ر� socket
	closesocket(_sock);
	// ���Windows socket����
	WSACleanup();
	cout << "�������" << endl;
	getchar();
	return 0;
}