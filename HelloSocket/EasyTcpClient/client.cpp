#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
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
	CMD_LOGINOUT,
	CMD_ERROR
};
//消息头
struct DataHeader
{
	short dataLength;    //数据长度 32767字节
	short cmd;
};

struct Login
{
	char userName[32];
	char Password[32];
};
struct Logout
{
	char userName[32];
};
struct LoginResult
{
	int result;
};
struct LogoutResult
{
	int result;
};

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
		// 3 输入请求命令
		char cmdBuf[128] = {};
		cout << "输入命令: ";
		cin >> cmdBuf;
		// 4 处理请求
		if (strcmp(cmdBuf, "exit") == 0)
		{
			break;
		}
		else if (0 == strcmp(cmdBuf,"login"))
		{
			Login _login = {"Evila","Evila_Password"};
			DataHeader _header = {};
			_header.dataLength = sizeof(Login);
			_header.cmd = CMD_LOGIN;
			// 5 向服务器发送请求命令 先发送包头
			send(_sock,(const char*)&_header, sizeof(DataHeader), 0);
			//再发送包体
			send(_sock, (const char*)&_login, sizeof(Login), 0);

			//6. 接受服务器信息 recv
			//先接收 返回数据的包头
			DataHeader returnHeader = {};
			LoginResult _lgRes = {};
			recv(_sock, (char*)&returnHeader, sizeof(DataHeader), 0);
			//再接收 返回数据的包体
			recv(_sock, (char*)&_lgRes, sizeof(LoginResult), 0);
			cout<<"LoginResult: " << _lgRes.result << endl;
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout _logout = {"Evila"};
			DataHeader _header = {};
			_header.dataLength = sizeof(Logout);
			_header.cmd = CMD_LOGINOUT;
			// 5 向服务器发送请求命令 先发送包头
			send(_sock, (const char*)&_header, sizeof(DataHeader), 0);
			//再发送包体
			send(_sock, (const char*)&_logout, sizeof(Logout), 0);

			//6. 接受服务器信息 recv
			//先接收 返回数据的包头
			DataHeader returnHeader = {};
			LogoutResult _lgRes = {};
			recv(_sock, (char*)&returnHeader, sizeof(DataHeader), 0);
			//再接收 返回数据的包体
			recv(_sock, (char*)&_lgRes, sizeof(LogoutResult), 0);
			cout << "LogoutResult: " << _lgRes.result << endl;
		}
		else
		{
			cout << "不支持的命令，请重新输入。" << endl;
		}
		//放到循环中 重复接受服务器的返回信息
		//6. 接受服务器信息 recv
		//char recvBuf[256] = {};
		//int nlen = recv(_sock, recvBuf, sizeof(recvBuf), 0); //返回值是接收数据的长度
		//if (nlen > 0)
		//{
		//	DataPackage* pDP = (DataPackage*)recvBuf;
		//	cout << "接收到数据： 年龄=" << pDP->age << ",名字=" << pDP->name << endl;
		//}
		//else
		//{
		//	cout << "ERROR: 数据传输失败..." << endl;
		//}
	}
	//7. 关闭 socket
	closesocket(_sock);
	// 清除Windows socket环境
	WSACleanup();
	cout << "任务结束" << endl;
	getchar();
	return 0;
}