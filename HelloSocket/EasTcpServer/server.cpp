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
	CMD_LOGIN_RESULT,
	CMD_LOGINOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
//消息头
struct DataHeader
{
	short dataLength;    //数据长度 32767字节
	short cmd;
};

struct Login: public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char Password[32];
};
struct Logout:public DataHeader
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
	//启动 windows socket 2.x 环境
	WORD versionCode = MAKEWORD(2, 2);	//创建一个版本号 
	WSADATA data;
	WSAStartup(versionCode, &data);  //启动Socket网络API的函数

	//(1) 用Socket API建立简易的TCP服务端

	//	1. 建立一个Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // ipv4 面向字节流的 tcp协议

	//	2. 绑定接受客户端连接的端口 bind

	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567); //端口号 host to net  sockaddr_in中port是USHORT类型 网络中port是 unsigend short类型 因此需要Htons进行转换
	//_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //服务器绑定的IP地址  127.0.0.1是本地地址
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; //不限定访问该服务端的IP
	if (bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)  //sockaddr 不利于编码  
	{
		cout << "ERROR: 绑定用于接受客户端连接的网络端口失败..." << endl;
	}
	else
	{
		cout << "SUCCESS: 绑定端口成功..." << endl;
	}
	//	3. 监听网络端口 listen

	if (listen(_sock, 5) == SOCKET_ERROR)//第二个参数 backbag 最大允许连接数量
	{
		cout << "ERROR: 监听用于接受客户端连接的网络端口失败..." << endl;
	}
	else
	{
		cout << "SUCCESS: 监听端口成功..." << endl;
	}

	//	4. 等待接受客户端连接 accept

	sockaddr_in _clientAddr = {};  
	int cliendAddrLen = sizeof(_clientAddr);
	SOCKET _clientSock = INVALID_SOCKET; // 初始化无效的socket 用来存储接入的客户端

	_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &cliendAddrLen);//当客户端接入时 会得到连入客户端的socket地址和长度
	if (INVALID_SOCKET == _clientSock) //接受到无效接入
	{
		cout << "ERROR: 接受到无效客户端SOCKET..." << endl;
	}
	else
	{
		cout << "新Client加入："<< "socket = "<< _clientSock <<" IP = " << inet_ntoa(_clientAddr.sin_addr) << endl;  //inet_ntoa 将ip地址转换成可读的字符串
	}


	char _recvBuf[128] = {};
	while (true)
	{
		DataHeader header = {};
		//5 首先接收数据包头
		int nlen = recv(_clientSock, (char*)&header, sizeof(header), 0); //接受客户端的数据 第一个参数应该是客户端的socket对象
		if (nlen <= 0)
		{
			//客户端退出
			cout << "客户端已退出，任务结束" << endl;
			break;
		}
		
		switch (header.cmd)
		{
			case CMD_LOGIN:
			{
				Login _login;
				recv(_clientSock, (char*)&_login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader),0);
				cout << "收到命令：CMD_LOGIN" << " 数据长度 = " << header.dataLength<<" UserName = "<< _login.userName<<" Password = "<<_login.Password << endl;
				//忽略了判断用户名密码是否正确的过程
				LoginResult _loginres;
				send(_clientSock, (char*)&_loginres, sizeof(LoginResult), 0);
			}break;
			case CMD_LOGINOUT:
			{
				Logout _logout;
				recv(_clientSock, (char*)&_logout + sizeof(DataHeader), sizeof(Logout) - sizeof(DataHeader), 0);
				cout << "收到命令：CMD_LOGOUT" << " 数据长度 = " << header.dataLength << " UserName = " << _logout.userName<< endl;
				LogoutResult _logoutres;
				send(_clientSock, (char*)&_logoutres, sizeof(LogoutResult), 0);
			}break;
			default:
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
				break;
		}
	}
	//	8. 关闭socket
	closesocket(_sock);
	// 清除Windows socket环境
	WSACleanup();
	cout << "任务结束" << endl;
	getchar();
	return 0;
}