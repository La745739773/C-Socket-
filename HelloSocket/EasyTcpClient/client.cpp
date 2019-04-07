#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<string>
#include<thread>	
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
	CMD_ERROR,
	CMD_NEWUSERJOIN,
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
struct NewUserJoin :public DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEWUSERJOIN;
		sockId = 0;
	}
	int sockId;
};


int processor(SOCKET _sock)
{
	char *szRecv = new char[1024];
	//5 ���Ƚ������ݰ�ͷ
	int nlen = recv(_sock, szRecv, sizeof(DataHeader), 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
	if (nlen <= 0)
	{
		//�ͻ����˳�
		cout << "�ͻ���:Socket = " << _sock << " ��������Ͽ����ӣ��������" << endl;
		return -1;
	}
	DataHeader* header = (DataHeader*)szRecv;
	switch (header->cmd)
	{
		case CMD_NEWUSERJOIN:
		{
			NewUserJoin _userJoin;
			recv(_sock, (char*)&_userJoin + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			cout << "�յ���������Ϣ: CMD_NEWUSERJOIN: SocketId = " << _userJoin.sockId << " ���ݳ���:" << header->dataLength << endl;
		}break;
		case CMD_LOGIN_RESULT:
		{
			LoginResult _lgRes;
			recv(_sock, (char*)&_lgRes + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			cout << "�յ���������Ϣ: CMD_LOGIN_RESULT: ��½״̬" << _lgRes.result << endl;
		}break;
		case CMD_LOGOUT_RESULT:
		{
			LogoutResult _lgRes;
			recv(_sock, (char*)&_lgRes + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			cout << "�յ���������Ϣ: CMD_LOGIN_RESULT: �ǳ�״̬" << _lgRes.result << endl;
		}break;
		default:
		{
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			send(_sock, (char*)&header, sizeof(DataHeader), 0);
		}
		break;
	}
	return 0;
}
bool g_bRun = true;
void cmdThread(SOCKET _sock)
{
	while (true)
	{
		// 3 ������������
		char cmdBuf[128] = {};
		cout << "��������: ";
		cin >> cmdBuf;
		// 4 ��������
		if (strcmp(cmdBuf, "exit") == 0)
		{
			cout << "�˳�cmdThread�߳�" << endl;
			g_bRun = false;
			return;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login _login;
			strcpy(_login.userName, "Evila");
			strcpy(_login.Password, "Evila_Password");
			// 5 �������������������
			send(_sock, (const char*)&_login, _login.dataLength, 0);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout _logout;
			strcpy(_logout.userName, "Evila");
			//5 �������������������
			send(_sock, (const char*)&_logout, _logout.dataLength, 0);
		}
		else
		{
			cout << "����֧�ֵ�����" << endl;
		}
	}
	return;
}


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
		cout << "Socket Id: " << _sock << endl;
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
	//�����߳�
	std::thread t1(cmdThread,_sock);
	t1.detach();//�����̷߳���

	while (g_bRun)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1,0 };
		int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			cout << "select�������" << endl;
			break;
		}
		if (FD_ISSET(_sock, &fdReads))	//���_sock��fdRead���棬����������ȴ�����
		{
			FD_CLR(_sock, &fdReads);

			if (processor(_sock) == -1)
			{
				cout << "Select�����ѽ���2" << endl;
				break;
			}
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






//// 3 ������������
//char cmdBuf[128] = {};
//cout << "��������: ";
//cin >> cmdBuf;
//// 4 ��������
//if (strcmp(cmdBuf, "exit") == 0)
//{
//	break;
//}
//else if (0 == strcmp(cmdBuf,"login"))
//{
//	Login _login;
//	strcpy(_login.userName, "Evila");
//	strcpy(_login.Password, "Evila_Password");
//	// 5 �������������������
//	send(_sock, (const char*)&_login, sizeof(Login), 0);

//	//6. ���ܷ�������Ϣ recv
//	LoginResult _lgRes;
//	recv(_sock, (char*)&_lgRes, sizeof(LoginResult), 0);
//	cout<<"LoginResult: " << _lgRes.result << endl;
//}
//else if (0 == strcmp(cmdBuf, "logout"))
//{
//	Logout _logout;
//	strcpy(_logout.userName, "Evila");
//	// 5 ������������������� 
//	send(_sock, (const char*)&_logout, sizeof(Logout), 0);

//	//6. ���ܷ�������Ϣ recv
//	LogoutResult _lgRes;
//	//��������
//	recv(_sock, (char*)&_lgRes, sizeof(LogoutResult), 0);
//	cout << "LogoutResult: " << _lgRes.result << endl;
//}
//else
//{
//	cout << "��֧�ֵ�������������롣" << endl;
//}