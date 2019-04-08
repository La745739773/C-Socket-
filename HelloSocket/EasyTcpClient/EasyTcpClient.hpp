#ifndef EasyTcpClient_hpp_
#define EasyTcpClient_hpp_
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<Windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib") //windows socket2 32的lib库
#else
	#include<unistd.h>
	#include<arpa/inet.h>
	#include<string.h>
	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR		   (-1)
#endif
#include<iostream>
#include"MessageHeadr.hpp"
using namespace std;
class EasyTcpClient
{
public:
	SOCKET _sock;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}
	//虚析构函数 直接的讲，C++中基类采用virtual虚析构函数是为了防止内存泄漏。
	//具体地说，如果派生类中申请了内存空间，并在其析构函数中对这些内存空间进行释放。
	//假设基类中采用的是非虚析构函数，当删除基类指针指向的派生类对象时就不会触发动态绑定，
	//因而只会调用基类的析构函数，而不会调用派生类的析构函数。那么在这种情况下，派生类中申请的空间就得不到释放从而产生内存泄漏。
	//所以，为了防止这种情况的发生，C++中基类的析构函数应采用virtual虚析构函数。
	virtual ~EasyTcpClient()  
	{
		closeSocket();
	}
	//初始化socket
	int initSocket()
	{
#ifdef _WIN32
		//启动Windows sock 2.x环境
		WORD versionCode = MAKEWORD(2, 2);	//创建一个版本号 
		WSADATA data;
		WSAStartup(versionCode, &data);  //启动Socket网络API的函数
#endif
		//1. 建立一个Socket
		if (_sock != INVALID_SOCKET)
		{
			cout << "<socket=>"<<_sock<<" 关闭了已有连接并重新建立连接" << endl;
			closeSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //用于网络链接的ipv4的socket TCP连接
		if (INVALID_SOCKET == _sock)	//SIocket 每一步都可以判断是否成功建立
		{
			cout << "ERROR: SOCKET 建立失败" << endl;
		}
		else
		{
			cout << "SUCCESS: SOCKET 建立成功" << endl;
		}
		return 0;
	}
	//连接服务器
	int ConnectServer(char* ip,unsigned short port)
	{
		if (_sock == INVALID_SOCKET)
		{
			initSocket();
		}
		//2. 连接服务器 connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port); //host to net short
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))	//SIocket 每一步都可以判断是否成功建立
		{
			cout << "ERROR: SOCKET 连接失败" << endl;
			getchar();
			return SOCKET_ERROR;
		}
		else
		{
			cout << "SUCCESS: SOCKET 连接成功" << endl;
			return 0;
		}
	}
	//关闭socket
	void closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. 关闭 socket
#ifdef _WIN32
		//清理win sock环境
		closesocket(_sock);
		// 清除Windows socket环境
		WSACleanup();
#else
		close(_sock);
#endif
		cout << "任务结束" << endl;
		getchar();
		_sock = INVALID_SOCKET;
	}

	//查询select网络消息
	bool onRun()
	{
		if (isRun())
		{
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 1,0 };
			int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
			if (ret < 0)
			{
				cout << "<socket = " << _sock << ">" << " select任务结束" << endl;
				return false;
			}
			if (FD_ISSET(_sock, &fdReads))	//如果_sock在fdRead里面，表明有接收数据等待处理
			{
				FD_CLR(_sock, &fdReads);

				if (RecvData() == -1)
				{
					cout << "<socket = " << _sock << ">" << " select任务结束2" << endl;
					return false;
				}
			}
			return true;
		}
		return false;
	}
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//接收数据  处理粘包、拆分包
	int RecvData()
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
		recv(_sock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return 0;
	}
	//响应网络消息
	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
			case CMD_NEWUSERJOIN:
			{
				NewUserJoin* _userJoin = (NewUserJoin*)header;
				cout << "收到服务器消息: CMD_NEWUSERJOIN: SocketId = " << _userJoin->sockId << " 数据长度:" << header->dataLength << endl;
			}break;
			case CMD_LOGIN_RESULT:
			{
				LoginResult* _lgRes = (LoginResult*)header;
				cout << "收到服务器消息: CMD_LOGIN_RESULT: 登陆状态" << _lgRes->result << endl;
			}break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult* _lgRes = (LogoutResult*)header;
				cout << "收到服务器消息: CMD_LOGIN_RESULT: 登出状态" << _lgRes->result << endl;
			}break;
			default:
			{
				header->cmd = CMD_ERROR;
				header->dataLength = 0;
				send(_sock, (char*)&header, sizeof(DataHeader), 0);
			}
			break;
		}
	}
	int SendData(DataHeader* header)
	{
		if (isRun() && header)
		{
			//cout << "发送数据给服务端" << endl;
			int ret = send(_sock, (const char*)header, header->dataLength, 0);
			return ret;
		}
		return SOCKET_ERROR;
	}
private:

};



#endif