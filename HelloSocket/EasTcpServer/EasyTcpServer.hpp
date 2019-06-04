#ifndef EasyTcpServer_hpp_
#define EasyTcpServer_hpp_
#include "MessageHeadr.hpp"
#include "CELLTimestamp.hpp"
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#ifndef FD_SETSIZE
	#define FD_SETSIZE      2506
	#endif /* FD_SETSIZE */
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
#include<cstring>
#include<thread>
#include<mutex>
#include<atomic>
#ifndef RECV_BUFF_SIZE
	#define RECV_BUFF_SIZE 10240
#endif



//客户端类   封装了socket  ip地址 地区 白名单等属性 
class ClientSocket
{
public:
	ClientSocket(SOCKET sock = INVALID_SOCKET)
	{
		_sockfd = sock;
		//_szMsgBuf = new char[RECV_BUFF_SIZE * 10];
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET getSock()
	{
		return _sockfd;
	}
	char* getMsgbuff()
	{
		return _szMsgBuf;
	}
	int getLstpos()
	{
		return _lastPos;
	}
	void setLstpos(int temp_pos)
	{
		_lastPos = temp_pos;
	}

	//发送数据
	int SendData(DataHeader* header)
	{
		if (header)
		{
			//cout << "发送数据给自己" << endl;
			int ret = send(_sockfd, (const char*)header, header->dataLength, 0);
			return ret;
		}
		return SOCKET_ERROR;
	}


private:
	SOCKET _sockfd;  //fd_set file desc set
	//消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 5];
	//消息缓冲区尾部
	int _lastPos;
};

//网络事件接口
class INetEvent
{
public:
	//客户端离开事件
	virtual void OnLeave(ClientSocket* pClient) = 0;	//线程不安全的函数，编写逻辑时应该考虑多线程的影响
	//客户端加入事件
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	//客户端消息响应事件
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader *header) = 0;
};
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_pthread = nullptr;
		_sock = sock;
		//_recvCount = 0;
		_pNetEvent = nullptr;
	}
	virtual ~CellServer()
	{
		delete _pthread;
		delete _pNetEvent;
		_closeSocket();
		_sock = INVALID_SOCKET;
	}
	void setEventObj(INetEvent* pEvent)
	{
		_pNetEvent = pEvent;
	}

	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	fd_set _fdRead_bak; //描述符集合的备份
	bool _clients_change;	//客户端列表是否有变化
	SOCKET _maxSock;
	bool OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			if (_clientBuff.size() > 0)
			{	//从缓冲队列取出客户端
				std::lock_guard<std::mutex> lg(_mutex);
				for (auto pClient : _clientBuff)
				{
					_clients.push_back(pClient);
				}
				_clientBuff.clear(); 
				_clients_change = true;
			}
			if (_clients.empty())
			{
				std::chrono::milliseconds t(10);
				std::this_thread::sleep_for(t);
				continue;
			}
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExpect;

			FD_ZERO(&fdRead);		//清空fd集合的数据
			if (_clients_change == true)
			{
				_clients_change = false;
				_maxSock = _clients[0]->getSock();
				for (size_t n = 0; n < _clients.size(); n++)
				{
					FD_SET(_clients[n]->getSock(), &fdRead);		//所有连入的客户端放入可读列表 保证recv不阻塞
					if (_maxSock < _clients[n]->getSock())
					{
						_maxSock = _clients[n]->getSock();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else
			{
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));	
			}
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExpect);
			//这个宏的功能是 将服务端的_sock 放到fdRead这个集合中 
			//当socket在listen状态，如果已经接收一个连接请求，这个socket会被标记为可读，例如一个accept会确保不会阻塞的完成
			//对于其他的socket，可读性意味着队列中的数据适合读，当调用recv后不会阻塞。
			//FD_SET(_sock, &fdRead);  //将服务端的socket放入可读列表，确保accept不阻塞
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExpect);

			//nfds第一个参数 是一个整数值 是指fd_set集合中所有socket值的范围 不是数量 
			timeval t = { 0,10 }; //select查询超时的时间  windows下的计时器 目前没有计算微秒  0表示select函数如果查询没有需要处理，立即返回
			int ret = select(_maxSock, &fdRead, 0, 0, nullptr);
			if (ret < 0)
			{
				std::cout << "select任务结束" << std::endl;
				return false;
			}
			for (int n = _clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(_clients[n]->getSock(), &fdRead))
				{
					if (RecvData(_clients[n]) == -1)
					{
						auto it = _clients.begin() + n;
						if (it != _clients.end())
						{
							_clients_change = true;
							if(_pNetEvent)
								_pNetEvent->OnLeave(_clients[n]);
							delete _clients[n];
							_clients.erase(it);
						}
					}
				}
			}
		}
	}

	char _szRecv[RECV_BUFF_SIZE] = {};
	int RecvData(ClientSocket* pClient)
	{
		//5 首先接收数据包头
		int nlen = recv(pClient->getSock(), _szRecv, RECV_BUFF_SIZE, 0); //接受客户端的数据 第一个参数应该是客户端的socket对象
		if (nlen <= 0)
		{
			//客户端退出
			//std::lock_guard<std::mutex> lg(_mutex);
			//_mutex.lock();
			//std::cout << "客户端:Socket = " << pClient->getSock() << " 已退出，任务结束" << std::endl;
			//_mutex.unlock();
			return -1;
		}
		//内存拷贝 将收取的数据拷贝到消息缓冲区中
		memcpy(pClient->getMsgbuff() + pClient->getLstpos(), _szRecv, nlen);
		//消息缓冲区尾部偏移量
		pClient->setLstpos(pClient->getLstpos() + nlen);
		while (pClient->getLstpos() >= sizeof(DataHeader))		//当前接收的消息长度大于数据头  循环处理粘包
		{
			DataHeader* header = (DataHeader*)pClient->getMsgbuff();
			if (pClient->getLstpos() >= header->dataLength)		//判断是否少包
			{
				//处理剩余未处理缓冲区数据的长度  先计算这个长度 因为下面会进行内存移动操作 因此会改变header中的值
				int nSize = pClient->getLstpos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient, header);
				//将剩余消息 前移方便下一次处理
				memcpy(pClient->getMsgbuff(), pClient->getMsgbuff() + header->dataLength, nSize);
				pClient->setLstpos(nSize);//位置迁移
			}
			else
			{
				//剩余数据不够一条完整消息 或少包
				break;
			}
		}
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader *header)
	{
		_pNetEvent->OnNetMsg(pClient, header);	

	}

	void _closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. 关闭 socket
#ifdef _WIN32
		//清理win sock环境
		closesocket(_sock);
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			closesocket(_clients[n]->getSock());
			delete _clients[n];
		}
		// 清除Windows socket环境
		WSACleanup();
#else
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			close(_clients[n]->getSock());
			delete _clients[n];
		}
#endif
		//std::cout << "任务结束" << std::endl;
		getchar();
		_clients.clear();
		_sock = INVALID_SOCKET;
	}

	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<std::mutex> lg(_mutex);	//自解锁 
		_clientBuff.push_back(pClient);
	}
	void Start()		//线程启动函数
	{
		//mem_fn 把成员函数转为函数对象，使用对象指针或对象引用进行绑定
		_pthread = new std::thread(std::mem_fn(&CellServer::OnRun),this); //C++调用成员函数的时候会 默认传递this指针
	}
	size_t getClientCount()
	{
		return _clients.size() + _clientBuff.size();
	}
private:
	SOCKET _sock;
	//正式客户队列
	std::vector<ClientSocket*> _clients;	//要处理的队列
	std::vector<ClientSocket*> _clientBuff; //缓冲队列
	std::mutex _mutex;
	std::thread* _pthread;
	INetEvent* _pNetEvent;//网络时间对象

};



class EasyTcpServer : public INetEvent
{
public:
	SOCKET _sock;
	//std::vector<ClientSocket*> _clients;	//用指针 防止爆栈 因为new的对象是在堆内存 不是栈内存
	CELLTimestamp _tTime;//每秒消息计时器
	std::vector<CellServer*> _cellServers;
	std::mutex _mutex;
public:
	std::atomic<int> _recvCount;
	std::atomic<int>_ClientCount; //客户端连接计数

public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_ClientCount = 0;
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
			std::cout << "<socket=>" << _sock << " 关闭了已有连接并重新建立连接" << std::endl;
			_closeSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //用于网络链接的ipv4的socket TCP连接
		if (INVALID_SOCKET == _sock)	//SIocket 每一步都可以判断是否成功建立
		{
			std::cout << "Socket = <" << _sock << ">" << " ERROR: SOCKET 建立失败" << std::endl;
		}
		else
		{
			std::cout << "Socket = <" << _sock << ">" << " SUCCESS: SOCKET 建立成功" << std::endl;
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
			_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		}
		else
		{
#ifdef _WIN32
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
			_sin.sin_addr.s_addr = INADDR_ANY;
#endif
		}
		
		if (bind(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)  //sockaddr 不利于编码  
		{
			std::cout << "Socket = <" << _sock << ">" << " ERROR: 绑定用于接受客户端连接的网络端口<"<<port<<">失败..." << std::endl;
			return -1;
		}
		else
		{
			std::cout << "Socket = <" << _sock << ">" << " SUCCESS: 绑定端口成功..." << std::endl;
		}
		return 0;
	}
	//监听端口
	int Listen(int _maxNumber)
	{
		if (listen(_sock, _maxNumber) == SOCKET_ERROR)//第二个参数 backbag 最大允许连接数量
		{
			std::cout << "Socket = <"<<_sock<<">"<<" ERROR: 监听用于接受客户端连接的网络端口失败..." << std::endl;
			return SOCKET_ERROR;
		}
		else
		{
			std::cout << "Socket = <" << _sock << ">" << " SUCCESS: 监听端口成功..." << std::endl;
			return 1;
		}
	}
	//创建执行数据收发处理的线程类对象
	void StartCellServers(int nCellServer)
	{
		for (int i = 0; i < nCellServer; i++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			//注册网络事件的接受对象
			ser->setEventObj(this);
			//启动消息处理线程
			ser->Start();
		}
	}

	//接收连接
	SOCKET Accept()
	{
		//	4. 等待接受客户端连接 accept
		sockaddr_in _clientAddr = {};
		int cliendAddrLen = sizeof(_clientAddr);
		SOCKET clientSock = INVALID_SOCKET; // 初始化无效的socket 用来存储接入的客户端
#ifdef _WIN32
		clientSock = accept(_sock, (sockaddr*)&_clientAddr, &cliendAddrLen);//当客户端接入时 会得到连入客户端的socket地址和长度
#else
		clientSock = accept(_sock, (sockaddr*)&_clientAddr, (socklen_t*)&cliendAddrLen);//当客户端接入时 会得到连入客户端的socket地址和长度
#endif
		if (INVALID_SOCKET == clientSock) //接受到无效接入
		{
			std::cout << "Socket = <" << _sock << " ERROR: 接受到无效客户端SOCKET..." << std::endl;
		}
		else
		{
			//将新客户端分配给客户数量最少的cellServer
			addClientToCellServer(new ClientSocket(clientSock));
			//获取ip地址的字符串 inet_ntoa(_clientAddr.sin_addr)
		}
		return clientSock;
	}
	void addClientToCellServer(ClientSocket *pClient)
	{
		//_clients.push_back(pClient);
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pCellServer->getClientCount() < pMinServer->getClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);	
		OnNetJoin(pClient);
	}

	//处理网络消息
	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExpect;

			FD_ZERO(&fdRead);		//清空fd集合的数据
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExpect);
			//这个宏的功能是 将服务端的_sock 放到fdRead这个集合中 
			//当socket在listen状态，如果已经接收一个连接请求，这个socket会被标记为可读，例如一个accept会确保不会阻塞的完成
			//对于其他的socket，可读性意味着队列中的数据适合读，当调用recv后不会阻塞。
			FD_SET(_sock, &fdRead);  //将服务端的socket放入可读列表，确保accept不阻塞
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExpect);

			//nfds第一个参数 是一个整数值 是指fd_set集合中所有socket值的范围 不是数量 
			timeval t = { 0,10 }; //select查询超时的时间  windows下的计时器 目前没有计算微秒  0表示select函数如果查询没有需要处理，立即返回
			//int ret = select(_sock+1, &fdRead, &fdWrite, &fdExpect, &t);
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			//cout << "select result = "<<ret << "count = "<< _nCount++ << endl;
			if (ret < 0)
			{
				std::cout << "Accept-Select任务结束" << std::endl;
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))	//判断_sock是否在fdRead中， 如果在 表明有客户端连接请求
			{
				FD_CLR(_sock, &fdRead);
				Accept();
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
	char _szRecv[RECV_BUFF_SIZE] = {};
	//计数并输出每秒收到的网络消息数量
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			//int _recvCount = 0;
			//for (auto ser : _cellServers)
			//{
			//	_recvCount += ser->_recvCount;
			//	ser->_recvCount = 0;
			//}
			std::cout <<"thread<"<<_cellServers.size()<<">,"<< "time<" << t1 << ">," << "socket<" << _sock << ">" << ",clients<"<<_ClientCount<<"个>" << ",recvCount<" << _recvCount << ">" << std::endl;
			_tTime.update();
			_recvCount = 0;
		}
	}

	virtual void OnLeave(ClientSocket* pClient)
	{
		_mutex.lock();
		_ClientCount--;
		//std::cout << "客户端:Socket = " << pClient->getSock() << " 已退出，任务结束" << std::endl;
		//for (int i = (int)_clients.size() - 1; i >= 0; i--)
		//{
		//	if (_clients[i] == pClient)
		//	{
		//		std::cout << "客户端:Socket = " << pClient->getSock() << " 已退出，任务结束" << std::endl;
		//		_clients.erase(_clients.begin() + i);
		//	}
		//}
		_mutex.unlock();
	}
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader *header)
	{
		_recvCount++;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* _login;
			_login = (Login*)header;
			//cout << "收到" << "socket = " << _clientSock << " 命令：CMD_LOGIN" << " 数据长度 = " << header->dataLength << " UserName = " << _login->userName << " Password = " << _login->Password << endl;
			//忽略了判断用户名密码是否正确的过程
			LoginResult _loginres;
			//pClient->SendData(&_loginres);
			//send(_clientSock, (char*)&_loginres, sizeof(LoginResult), 0);
		}break;
		case CMD_LOGINOUT:
		{
			Logout *_logout;
			_logout = (Logout*)header;
			std::cout << "收到" << "socket = " << pClient->getSock() << " 命令：CMD_LOGOUT" << " 数据长度 = " << header->dataLength << " UserName = " << _logout->userName << std::endl;
			LogoutResult _logoutres;
			send(pClient->getSock(), (char*)&_logoutres, sizeof(LogoutResult), 0);
		}break;
		default:
		{
			std::cout << "收到服务器消息: 未定义消息:" << " 数据长度:" << header->dataLength << std::endl;
			//header->cmd = CMD_ERROR;
			//send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
		}
		break;
		}
	}
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_ClientCount++;
	}
	void _closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. 关闭 socket
#ifdef _WIN32
		//清理win sock环境
		closesocket(_sock);
		//for (int n = _clients.size() - 1; n >= 0; n--)
		//{
		//	closesocket(_clients[n]->getSock());
		//	delete _clients[n];
		//}
		// 清除Windows socket环境
		WSACleanup();
#else
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			close(_clients[n]->getSock());
			delete _clients[n];
		}
#endif
		//std::cout << "任务结束" << std::endl;
		getchar();
		//_clients.clear();
		_sock = INVALID_SOCKET;
	}
};

#endif // !EasyTcpServer_hpp_
