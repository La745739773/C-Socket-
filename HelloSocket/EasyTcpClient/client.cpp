#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<string>
using namespace std;
#pragma comment(lib,"ws2_32.lib") //windows socket2 32的lib库

//一定要保证服务端和客户端（操作系统）中 数据结构字节顺序和大小保证一致 
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
//消息头
struct DataHeader
{
	short dataLength;    //数据长度 32767字节
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
		dataLength = sizeof(LogoutResult);
		cmd = CMD_NEWUSERJOIN;
		sockId = 0;
	}
	int sockId;
};


int processor(SOCKET _sock)
{
	char *szRecv = new char[1024];
	//5 首先接收数据包头
	int nlen = recv(_sock, szRecv, sizeof(DataHeader), 0); //接受客户端的数据 第一个参数应该是客户端的socket对象
	if (nlen <= 0)
	{
		//客户端退出
		cout << "客户端:Socket = " << _sock << " 与服务器断开连接，任务结束" << endl;
		return -1;
	}
	DataHeader* header = (DataHeader*)szRecv;
	switch (header->cmd)
	{
		case CMD_NEWUSERJOIN:
		{
			NewUserJoin _userJoin;
			recv(_sock, (char*)&_userJoin + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			cout << "收到服务器消息: CMD_NEWUSERJOIN:" << _userJoin.sockId << endl;
		}break;
		case CMD_LOGIN_RESULT:
		{
			LoginResult _lgRes;
			recv(_sock, (char*)&_lgRes + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			cout << "收到服务器消息: CMD_LOGIN_RESULT:" << _lgRes.result << endl;
		}break;
		case CMD_LOGOUT_RESULT:
		{
			LogoutResult _lgRes;
			recv(_sock, (char*)&_lgRes + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			cout << "收到服务器消息: CMD_LOGIN_RESULT:" << _lgRes.result << endl;
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


int main()
{
	//启动 windows socket 2.x 环境
	WORD versionCode = MAKEWORD(2, 2);	//创建一个版本号 
	WSADATA data;
	WSAStartup(versionCode, &data);  //启动Socket网络API的函数

	//1. 建立一个Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0); //用于网络链接的ipv4的socket

	if (INVALID_SOCKET == _sock)	//SIocket 每一步都可以判断是否成功建立
	{
		cout << "ERROR: SOCKET 建立失败" << endl;
	}
	else
	{
		cout << "SUCCESS: SOCKET 建立成功" << endl;
	}

	//2. 连接服务器 connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567); //host to net short
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))	//SIocket 每一步都可以判断是否成功建立
	{
		cout << "ERROR: SOCKET 连接失败" << endl;
		getchar();
		return 0;
	}
	else
	{
		cout << "SUCCESS: SOCKET 连接成功" << endl;
	}

	while (true)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1,0 };
		int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0)
		{
			cout << "select任务结束" << endl;
			break;
		}
		if (FD_ISSET(_sock, &fdReads))	//如果_sock在fdRead里面，表明有需求等待处理
		{
			FD_CLR(_sock, &fdReads);

			if (processor(_sock) == -1)
			{
				cout << "Select任务已结束2" << endl;
				break;
			}
		}
		Login login;
		strcpy(login.userName, "Evila");
		strcpy(login.Password, "Evila_passWord");
		send(_sock, (const char*)&login, login.dataLength, 0);
		Sleep(1000);
	}
	//7. 关闭 socket
	closesocket(_sock);
	// 清除Windows socket环境
	WSACleanup();
	cout << "任务结束" << endl;
	getchar();
	return 0;
}






//// 3 输入请求命令
//char cmdBuf[128] = {};
//cout << "输入命令: ";
//cin >> cmdBuf;
//// 4 处理请求
//if (strcmp(cmdBuf, "exit") == 0)
//{
//	break;
//}
//else if (0 == strcmp(cmdBuf,"login"))
//{
//	Login _login;
//	strcpy(_login.userName, "Evila");
//	strcpy(_login.Password, "Evila_Password");
//	// 5 向服务器发送请求命令
//	send(_sock, (const char*)&_login, sizeof(Login), 0);

//	//6. 接受服务器信息 recv
//	LoginResult _lgRes;
//	recv(_sock, (char*)&_lgRes, sizeof(LoginResult), 0);
//	cout<<"LoginResult: " << _lgRes.result << endl;
//}
//else if (0 == strcmp(cmdBuf, "logout"))
//{
//	Logout _logout;
//	strcpy(_logout.userName, "Evila");
//	// 5 向服务器发送请求命令 
//	send(_sock, (const char*)&_logout, sizeof(Logout), 0);

//	//6. 接受服务器信息 recv
//	LogoutResult _lgRes;
//	//返回数据
//	recv(_sock, (char*)&_lgRes, sizeof(LogoutResult), 0);
//	cout << "LogoutResult: " << _lgRes.result << endl;
//}
//else
//{
//	cout << "不支持的命令，请重新输入。" << endl;
//}