#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<string>
using namespace std;
#pragma comment(lib,"ws2_32.lib") //windows socket2 32的lib库

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
		else
		{
			// 5 向服务器发送请求命令
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}
		//放到循环中 重复接受服务器的返回信息
		//6. 接受服务器信息 recv
		char recvBuf[256] = {};
		int nlen = recv(_sock, recvBuf, sizeof(recvBuf), 0); //返回值是接收数据的长度
		if (nlen > 0)
		{
			cout << recvBuf << endl;
		}
		else
		{
			cout << "ERROR: 数据传输失败..." << endl;
		}
	}
	//7. 关闭 socket
	closesocket(_sock);
	// 清除Windows socket环境
	WSACleanup();
	cout << "任务结束" << endl;
	getchar();
	return 0;
}