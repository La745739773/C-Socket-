#ifndef EasyTcpServer_hpp_
#define EasyTcpServer_hpp_
#include "MessageHeadr.hpp"
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
#include<vector>
using namespace std;

class EasyTcpServer
{
public:
	SOCKET _sock;
	std::vector<SOCKET> g_clinets;
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpServer()
	{
		_closeSocket();
	}
	void initSocket()
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
			cout << "<socket=>" << _sock << " 关闭了已有连接并重新建立连接" << endl;
			_closeSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //用于网络链接的ipv4的socket TCP连接
		if (INVALID_SOCKET == _sock)	//SIocket 每一步都可以判断是否成功建立
		{
			cout << "Socket = <" << _sock << ">" << " ERROR: SOCKET 建立失败" << endl;
		}
		else
		{
			cout << "Socket = <" << _sock << ">" << " SUCCESS: SOCKET 建立成功" << endl;
		}
		return ;
	}

	//绑定端口
	int Bind(const char* ip,unsigned short port)
	{
		if (_sock == INVALID_SOCKET)
		{
			initSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		if (ip)
		{
#ifdef _WIN32
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
			_sin.sin_addr.S_addr = inet_addr(ip);
#endif
		}
		else
		{
#ifdef _WIN32
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
			_sin.sin_addr.S_addr = INADDR_ANY;
#endif
		}
		if (bind(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)  //sockaddr 不利于编码  
		{
			cout << "Socket = <" << _sock << ">" << " ERROR: 绑定用于接受客户端连接的网络端口<"<<port<<">失败..." << endl;
			return -1;
		}
		else
		{
			cout << "Socket = <" << _sock << ">" << " SUCCESS: 绑定端口成功..." << endl;
		}
		return 0;
	}
	//监听端口
	int Listen(int _maxNumber)
	{
		if (listen(_sock, _maxNumber) == SOCKET_ERROR)//第二个参数 backbag 最大允许连接数量
		{
			cout << "Socket = <"<<_sock<<">"<<" ERROR: 监听用于接受客户端连接的网络端口失败..." << endl;
			return SOCKET_ERROR;
		}
		else
		{
			cout << "Socket = <" << _sock << ">" << " SUCCESS: 监听端口成功..." << endl;
			return 1;
		}
	}
	//接收连接
	SOCKET Accept()
	{
		//	4. 等待接受客户端连接 accept
		sockaddr_in _clientAddr = {};
		int cliendAddrLen = sizeof(_clientAddr);
		SOCKET _clientSock = INVALID_SOCKET; // 初始化无效的socket 用来存储接入的客户端

		_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &cliendAddrLen);//当客户端接入时 会得到连入客户端的socket地址和长度
		if (INVALID_SOCKET == _clientSock) //接受到无效接入
		{
			cout << "Socket = <" << _sock << " ERROR: 接受到无效客户端SOCKET..." << endl;
		}
		else
		{
			cout << "新Client加入：" << "socket = " << _clientSock << " IP = " << inet_ntoa(_clientAddr.sin_addr) << endl;  //inet_ntoa 将ip地址转换成可读的字符串
			for (int n = g_clinets.size() - 1; n >= 0; n--)
			{
				NewUserJoin userJoin;
				userJoin.cmd = CMD_NEWUSERJOIN;
				userJoin.sockId = _clientSock;
				SendData2All(&userJoin);
				//send(g_clinets[n], (const char*)&userJoin, userJoin.dataLength, 0);
			}
			g_clinets.push_back(_clientSock);
		}
		return _clientSock;
	}
	//处理网络消息
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExpect;

			FD_ZERO(&fdRead);		//清空fd集合的数据
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExpect);
			//这个宏的功能是 将服务端的_sock 放到fdRead这个集合中 
			//当socket在listen状态，如果已经接收一个连接请求，这个socket会被标记为可读，例如一个accept会确保不会阻塞的完成
			//对于其他的socket，可读性意味着队列中的数据适合读，当调用recv后不会阻塞。
			FD_SET(_sock, &fdRead);  //将服务端的socket放入可读列表，确保accept不阻塞
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExpect);

			for (size_t n = 0; n < g_clinets.size(); n++)
			{
				FD_SET(g_clinets[n], &fdRead);		//所有连入的客户端放入可读列表 保证recv不阻塞
			}

			//nfds第一个参数 是一个整数值 是指fd_set集合中所有socket值的范围 不是数量 
			timeval t = { 1,0 }; //select查询超时的时间  windows下的计时器 目前没有计算微秒  0表示select函数如果查询没有需要处理，立即返回
			int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExpect, &t);
			if (ret < 0)
			{
				cout << "select任务结束" << endl;
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))	//判断_sock是否在fdRead中， 如果在 表明有客户端连接请求
			{
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			for (size_t n = 0; n < fdRead.fd_count; n++)
			{
				if (RecvData(fdRead.fd_array[n]) == -1)//processor函数是处理命令的逻辑 recv接到的数据并做出相应的判断和输出日志
				{
					auto it = find(g_clinets.begin(), g_clinets.end(), fdRead.fd_array[n]);
					if (it != g_clinets.end())
						g_clinets.erase(it);
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

	//select网络模型
	//接收数据
	int RecvData(SOCKET _clientSock)
	{
		char *szRecv = new char[1024];
		//5 首先接收数据包头
		int nlen = recv(_clientSock, szRecv, sizeof(DataHeader), 0); //接受客户端的数据 第一个参数应该是客户端的socket对象
		if (nlen <= 0)
		{
			//客户端退出
			cout << "客户端:Socket = " << _clientSock << " 已退出，任务结束" << endl;
			return -1;
		}
		DataHeader* header = (DataHeader*)szRecv;
		recv(_clientSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_clientSock, header);
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(SOCKET _clientSock, DataHeader *header)
	{
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
				Login* _login;
				_login = (Login*)header;
				cout << "收到" << "socket = " << _clientSock << " 命令：CMD_LOGIN" << " 数据长度 = " << header->dataLength << " UserName = " << _login->userName << " Password = " << _login->Password << endl;
				//忽略了判断用户名密码是否正确的过程
				LoginResult _loginres;
				send(_clientSock, (char*)&_loginres, sizeof(LoginResult), 0);
			}break;
			case CMD_LOGINOUT:
			{
				Logout *_logout;
				_logout = (Logout*)header;
				cout << "收到" << "socket = " << _clientSock << " 命令：CMD_LOGOUT" << " 数据长度 = " << header->dataLength << " UserName = " << _logout->userName << endl;
				LogoutResult _logoutres;
				send(_clientSock, (char*)&_logoutres, sizeof(LogoutResult), 0);
			}break;
			default:
			{
				header->cmd = CMD_ERROR;
				header->dataLength = 0;
				send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
			}
			break;
		}
	}

	//发送到指定的socket数据
	int SendData(SOCKET _clinetSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			//cout << "发送数据给服务端" << endl;
			int ret = send(_clinetSock, (const char*)header, header->dataLength, 0);
			return ret;
		}
		return SOCKET_ERROR;
	}

	void SendData2All(DataHeader *header)
	{
		if (isRun() && header)
		{
			//cout << "发送数据给服务端" << endl;
			for (int n = g_clinets.size() - 1; n >= 0; n--)
			{
				NewUserJoin userJoin;
				userJoin.cmd = CMD_NEWUSERJOIN;
				userJoin.sockId = g_clinets[n];
				//send(g_clinets[n], (const char*)&userJoin, userJoin.dataLength, 0);
				SendData(g_clinets[n], header);
			}
		}
		return;
	}


	void _closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. 关闭 socket
#ifdef _WIN32
		//清理win sock环境
		closesocket(_sock);
		for (int n = g_clinets.size() - 1; n >= 0; n--)
		{
			closesocket(g_clinets[n]);
		}
		// 清除Windows socket环境
		WSACleanup();
#else
		close(_sock);
#endif
		cout << "任务结束" << endl;
		getchar();
		_sock = INVALID_SOCKET;
	}
};

#endif // !EasyTcpServer_hpp_
