#ifndef EasyTcpServer_hpp_
#define EasyTcpServer_hpp_
#include<memory>
#include <functional>
#include "MessageHeadr.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
#include "CELLObjectPool.hpp"


#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#ifndef FD_SETSIZE
	#define FD_SETSIZE      2506
	#endif /* FD_SETSIZE */
	#include<Windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib") //windows socket2 32��lib��
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
	#define RECV_BUFF_SIZE 10240*5
	#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif


typedef std::shared_ptr<DataHeader> DataHeaderPtr;
//�ͻ�����   ��װ��socket  ip��ַ ���� ������������ 
class ClientSocket : public ObjectPoolBase<ClientSocket,2000>
{
public:
	ClientSocket(SOCKET sock = INVALID_SOCKET)
	{
		_sockfd = sock;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastPos = 0;
		_lastSendPos = 0;
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

	//��������
	int SendData(DataHeaderPtr header)
	{
		int res = SOCKET_ERROR;
		//Ҫ���͵����ݳ���
		int nSendlen = header->dataLength;
		//Ҫ���͵�����
		const char* pSendData = (const char*)header.get();
		while (true)
		{
			if (_lastSendPos + nSendlen >= SEND_BUFF_SIZE)
			{//64k 65536�ֽ� TCP�ṩ�Ļ�������С
			 //������Կ��������ݳ���
				int nCpyLen = SEND_BUFF_SIZE - _lastSendPos;
				//��������
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCpyLen);
				//ʣ������λ�� ��Ҫ���͵����ݽ���ƫ��
				pSendData += nCpyLen;
				//ʣ�����ݳ���
				nSendlen -= nCpyLen;
				res = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);

				_lastSendPos = 0;
				if (res == SOCKET_ERROR)
				{
					return res;
				}
			}
			else
			{
				//��������
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendlen);
				_lastSendPos += nSendlen;
				nSendlen = 0;
				break;
			}
		}
		return res;
	}


private:
	SOCKET _sockfd;  //fd_set file desc set
	//��Ϣ������
	char _szMsgBuf[RECV_BUFF_SIZE];
	//��Ϣ������β��
	int _lastPos;
	//���ͻ�����
	char _szSendBuf[RECV_BUFF_SIZE];
	//���ͻ�����β��
	int _lastSendPos;
};
typedef std::shared_ptr<ClientSocket> ClientSocketPtr;
class CellSendMsg2ClientTask;
typedef std::shared_ptr<CellSendMsg2ClientTask> CellSendMsg2ClientTaskPtr;
class CellServer;
typedef std::shared_ptr<CellServer> CellServerPtr;

//�����¼��ӿ�
class INetEvent
{
public:
	//�ͻ����뿪�¼� ClientSocketPtr pClient ����������¼��Ķ���
	virtual void OnLeave(ClientSocketPtr& pClient) = 0;	//�̲߳���ȫ�ĺ�������д�߼�ʱӦ�ÿ��Ƕ��̵߳�Ӱ��
														//�ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocketPtr& pClient) = 0;
	//�ͻ�����Ϣ��Ӧ�¼�
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, DataHeader *header) = 0;
	//recv�¼�
	virtual void OnNetRecv(ClientSocketPtr& pClient) = 0;
	////send�¼�
	//virtual void OnNetSend(ClientSocketPtr pClient) = 0;
};

//������Ϣ��������
class CellSendMsg2ClientTask : public CellTask
{
private:
	ClientSocketPtr _pClient;	//���͵��ͻ���Ŀ�ĵ�
	DataHeaderPtr _pHeader;	//Ҫ���͵�����
public:
	CellSendMsg2ClientTask(ClientSocketPtr pClient, DataHeaderPtr header)
	{
		_pClient = pClient;
		_pHeader = header;
	}
	//ִ�����񷽷�
	void doTask()
	{
		_pClient->SendData(_pHeader);
		//delete _pHeader;
	}

};
//������Ϣ���շ�����
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

	fd_set _fdRead_bak; //���������ϵı���
	bool _clients_change;	//�ͻ����б��Ƿ��б仯
	SOCKET _maxSock;
	bool OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			if (_clientBuff.size() > 0)
			{	//�ӻ������ȡ���ͻ���
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

			FD_ZERO(&fdRead);		//���fd���ϵ�����
			if (_clients_change == true)
			{
				_clients_change = false;
				_maxSock = _clients[0]->getSock();
				for (size_t n = 0; n < _clients.size(); n++)
				{
					FD_SET(_clients[n]->getSock(), &fdRead);		//��������Ŀͻ��˷���ɶ��б� ��֤recv������
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
			//�����Ĺ����� ������˵�_sock �ŵ�fdRead��������� 
			//��socket��listen״̬������Ѿ�����һ�������������socket�ᱻ���Ϊ�ɶ�������һ��accept��ȷ���������������
			//����������socket���ɶ�����ζ�Ŷ����е������ʺ϶���������recv�󲻻�������
			//FD_SET(_sock, &fdRead);  //������˵�socket����ɶ��б�ȷ��accept������
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExpect);

			//nfds��һ������ ��һ������ֵ ��ָfd_set����������socketֵ�ķ�Χ �������� 
			timeval t = { 0,10 }; //select��ѯ��ʱ��ʱ��  windows�µļ�ʱ�� Ŀǰû�м���΢��  0��ʾselect���������ѯû����Ҫ������������
			int ret = select(_maxSock, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				std::cout << "select�������" << std::endl;
				return false;
			}
#ifdef _WIN32
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
							if (_pNetEvent)
								_pNetEvent->OnLeave(_clients[n]);
							//delete _clients[n];
							_clients.erase(it);
						}
					}
				}
			}
#else
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
							if (_pNetEvent)
								_pNetEvent->OnLeave(_clients[n]);
							//delete _clients[n];
							_clients.erase(it);
						}
					}
				}
			}
#endif // _WIN32


		}
	}

	//char _szRecv[RECV_BUFF_SIZE] = {};
	int RecvData(ClientSocketPtr pClient)
	{
		//5 ���Ƚ������ݰ�ͷ
		char *szRecv = pClient->getMsgbuff() + pClient->getLstpos();
		int nlen = recv(pClient->getSock(), szRecv, RECV_BUFF_SIZE - pClient->getLstpos(), 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
		_pNetEvent->OnNetRecv(pClient);
		if (nlen <= 0)
		{
			return -1;
		}
		//�ڴ濽�� ����ȡ�����ݿ�������Ϣ��������
		memcpy(pClient->getMsgbuff() + pClient->getLstpos(), szRecv, nlen);
		//��Ϣ������β��ƫ����
		pClient->setLstpos(pClient->getLstpos() + nlen);
		while (pClient->getLstpos() >= sizeof(DataHeader))		//��ǰ���յ���Ϣ���ȴ�������ͷ  ѭ������ճ��
		{
			DataHeader* header = (DataHeader*)pClient->getMsgbuff();
			if (pClient->getLstpos() >= header->dataLength)		//�ж��Ƿ��ٰ�
			{
				//����ʣ��δ�����������ݵĳ���  �ȼ���������� ��Ϊ���������ڴ��ƶ����� ��˻�ı�header�е�ֵ
				int nSize = pClient->getLstpos() - header->dataLength;
				//����������Ϣ
				OnNetMsg(pClient, header);
				//��ʣ����Ϣ ǰ�Ʒ�����һ�δ���
				memcpy(pClient->getMsgbuff(), pClient->getMsgbuff() + header->dataLength, nSize);
				pClient->setLstpos(nSize);//λ��Ǩ��
			}
			else
			{
				//ʣ�����ݲ���һ��������Ϣ ���ٰ�
				break;
			}
		}
		return 0;
	}

	//��Ӧ������Ϣ
	virtual void OnNetMsg(ClientSocketPtr pClient, DataHeader *header)
	{
		_pNetEvent->OnNetMsg(this,pClient, header);	

	}

	void _closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. �ر� socket
#ifdef _WIN32
		//����win sock����
		closesocket(_sock);
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			closesocket(_clients[n]->getSock());
			//delete _clients[n];
		}
		// ���Windows socket����
		WSACleanup();
#else
		close(_sock);
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			close(_clients[n]->getSock());
			//delete _clients[n];
		}
#endif
		//std::cout << "�������" << std::endl;
		getchar();
		_clients.clear();
		_sock = INVALID_SOCKET;
	}

	void addClient(ClientSocketPtr pClient)
	{
		std::lock_guard<std::mutex> lg(_mutex);	//�Խ��� 
		_clientBuff.push_back(pClient);
	}
	void Start()		//�߳���������
	{
		//mem_fn �ѳ�Ա����תΪ��������ʹ�ö���ָ���������ý��а�
		_pthread = new std::thread(std::mem_fn(&CellServer::OnRun),this); //C++���ó�Ա������ʱ��� Ĭ�ϴ���thisָ��
		_taskServer.Start();
	}
	size_t getClientCount()
	{
		return _clients.size() + _clientBuff.size();
	}


	void addSendTask(ClientSocketPtr pClient, DataHeaderPtr header)
	{
		CellSendMsg2ClientTaskPtr pTask = std::make_shared<CellSendMsg2ClientTask>(pClient, header);
		//CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
		_taskServer.addTask((CellTaskPtr&)pTask);
	}

private:
	SOCKET _sock;
	//��ʽ�ͻ�����
	std::vector<ClientSocketPtr> _clients;	//Ҫ����Ķ���
	std::vector<ClientSocketPtr> _clientBuff; //�������
	std::mutex _mutex;
	std::thread* _pthread;
	INetEvent* _pNetEvent;//����ʱ�����
	CellTaskServer _taskServer;//
};

class EasyTcpServer : public INetEvent
{
public:
	SOCKET _sock;
	//��ָ�� ��ֹ��ջ ��Ϊnew�Ķ������ڶ��ڴ� ����ջ�ڴ�
	CELLTimestamp _tTime;//ÿ����Ϣ��ʱ��
	std::vector<CellServerPtr> _cellServers;
	std::mutex _mutex;

public:
	std::atomic<int> _recvCount;
	std::atomic<int>_ClientCount; //�ͻ������Ӽ���
	std::atomic<int>_msgCount;
	//std::atomic<int>_sendCount;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_ClientCount = 0;
		_msgCount = 0;
		//_sendCount = 0;
	}
	virtual ~EasyTcpServer()
	{
		_closeSocket();
	}
	void initSocket()
	{
#ifdef _WIN32
		//����Windows sock 2.x����
		WORD versionCode = MAKEWORD(2, 2);	//����һ���汾�� 
		WSADATA data;
		WSAStartup(versionCode, &data);  //����Socket����API�ĺ���
#endif
										 //1. ����һ��Socket
		if (_sock != INVALID_SOCKET)
		{
			std::cout << "<socket=>" << _sock << " �ر����������Ӳ����½�������" << std::endl;
			_closeSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //�����������ӵ�ipv4��socket TCP����
		if (INVALID_SOCKET == _sock)	//SIocket ÿһ���������ж��Ƿ�ɹ�����
		{
			std::cout << "Socket = <" << _sock << ">" << " ERROR: SOCKET ����ʧ��" << std::endl;
		}
		else
		{
			std::cout << "Socket = <" << _sock << ">" << " SUCCESS: SOCKET �����ɹ�" << std::endl;
		}
		return ;
	}

	//�󶨶˿�
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
		
		if (bind(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)  //sockaddr �����ڱ���  
		{
			std::cout << "Socket = <" << _sock << ">" << " ERROR: �����ڽ��ܿͻ������ӵ�����˿�<"<<port<<">ʧ��..." << std::endl;
			return -1;
		}
		else
		{
			std::cout << "Socket = <" << _sock << ">" << " SUCCESS: �󶨶˿ڳɹ�..." << std::endl;
		}
		return 0;
	}
	//�����˿�
	int Listen(int _maxNumber)
	{
		if (listen(_sock, _maxNumber) == SOCKET_ERROR)//�ڶ������� backbag ���������������
		{
			std::cout << "Socket = <"<<_sock<<">"<<" ERROR: �������ڽ��ܿͻ������ӵ�����˿�ʧ��..." << std::endl;
			return SOCKET_ERROR;
		}
		else
		{
			std::cout << "Socket = <" << _sock << ">" << " SUCCESS: �����˿ڳɹ�..." << std::endl;
			return 1;
		}
	}
	//����ִ�������շ�������߳������
	void StartCellServers(int nCellServer)
	{
		for (int i = 0; i < nCellServer; i++)
		{
			auto ser = std::make_shared<CellServer>(_sock);
			_cellServers.push_back(ser);
			//ע�������¼��Ľ��ܶ���
			ser->setEventObj(this);
			//������Ϣ�����߳�
			ser->Start();
		}
	}

	//��������
	SOCKET Accept()
	{
		//	4. �ȴ����ܿͻ������� accept
		sockaddr_in _clientAddr = {};
		int cliendAddrLen = sizeof(_clientAddr);
		SOCKET clientSock = INVALID_SOCKET; // ��ʼ����Ч��socket �����洢����Ŀͻ���
#ifdef _WIN32
		clientSock = accept(_sock, (sockaddr*)&_clientAddr, &cliendAddrLen);//���ͻ��˽���ʱ ��õ�����ͻ��˵�socket��ַ�ͳ���
#else
		clientSock = accept(_sock, (sockaddr*)&_clientAddr, (socklen_t*)&cliendAddrLen);//���ͻ��˽���ʱ ��õ�����ͻ��˵�socket��ַ�ͳ���
#endif
		if (INVALID_SOCKET == clientSock) //���ܵ���Ч����
		{
			std::cout << "Socket = <" << _sock << " ERROR: ���ܵ���Ч�ͻ���SOCKET..." << std::endl;
		}
		else
		{
			//���¿ͻ��˷�����ͻ��������ٵ�cellServer
			ClientSocketPtr c(new ClientSocket(clientSock));	
			//����û���� make_sharedȥ��������ָ�������Ϊmake_shared���������Ķ��������ָ������Ҫ�������Դ��ͬ������룬�Ӷ�������뵽�������
			//ʹ�����ַ�ʽ��������ָ��ʱ������������Ķ���ʹ�������ָ���Ƿֿ����ģ���˻���뵽����أ�ֻ��new�Ĵ�������
			addClientToCellServer(c);
			//��ȡip��ַ���ַ��� inet_ntoa(_clientAddr.sin_addr)
		}
		return clientSock;
	}
	void addClientToCellServer(ClientSocketPtr pClient)
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

	//����������Ϣ
	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExpect;

			FD_ZERO(&fdRead);		//���fd���ϵ�����
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExpect);
			//�����Ĺ����� ������˵�_sock �ŵ�fdRead��������� 
			//��socket��listen״̬������Ѿ�����һ�������������socket�ᱻ���Ϊ�ɶ�������һ��accept��ȷ���������������
			//����������socket���ɶ�����ζ�Ŷ����е������ʺ϶���������recv�󲻻�������
			FD_SET(_sock, &fdRead);  //������˵�socket����ɶ��б�ȷ��accept������
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExpect);

			//nfds��һ������ ��һ������ֵ ��ָfd_set����������socketֵ�ķ�Χ �������� 
			timeval t = { 0,10 }; //select��ѯ��ʱ��ʱ��  windows�µļ�ʱ�� Ŀǰû�м���΢��  0��ʾselect���������ѯû����Ҫ������������
			//int ret = select(_sock+1, &fdRead, &fdWrite, &fdExpect, &t);
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			//cout << "select result = "<<ret << "count = "<< _nCount++ << endl;
			if (ret < 0)
			{
				std::cout << "Accept-Select�������" << std::endl;
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))	//�ж�_sock�Ƿ���fdRead�У� ����� �����пͻ�����������
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

	//select����ģ��
	//��������
	char _szRecv[RECV_BUFF_SIZE] = {};
	//���������ÿ���յ���������Ϣ����
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
			std::cout <<"thread<"<<_cellServers.size()<<">,"<< "time<" << t1 << ">," << "socket<" << _sock << ">" << ",clients<"<<_ClientCount<<"��>" << ",recvCount<" << _recvCount << ">" <<",msg<"<<_msgCount<<">"<< std::endl;
			_tTime.update();
			_msgCount = 0;
			_recvCount = 0;
		}
	}

	virtual void OnLeave(ClientSocketPtr& pClient)
	{
		_mutex.lock();
		_ClientCount--;
		//std::cout << "�ͻ���:Socket = " << pClient->getSock() << " ���˳����������" << std::endl;
		//for (int i = (int)_clients.size() - 1; i >= 0; i--)
		//{
		//	if (_clients[i] == pClient)
		//	{
		//		std::cout << "�ͻ���:Socket = " << pClient->getSock() << " ���˳����������" << std::endl;
		//		_clients.erase(_clients.begin() + i);
		//	}
		//}
		_mutex.unlock();
	}
	virtual void OnNetMsg(CellServer* pCellServer,ClientSocketPtr& pClient, DataHeader *header)
	{
		_msgCount++;
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* _login;
			_login = (Login*)header;
			//cout << "�յ�" << "socket = " << _clientSock << " ���CMD_LOGIN" << " ���ݳ��� = " << header->dataLength << " UserName = " << _login->userName << " Password = " << _login->Password << endl;
			//�������ж��û��������Ƿ���ȷ�Ĺ���
			DataHeaderPtr pLr = std::make_shared<LoginResult>();
			//LoginResult* _loginres = new LoginResult();
			pCellServer->addSendTask(pClient, pLr);
			//pClient->SendData(&_loginres);
			//send(_clientSock, (char*)&_loginres, sizeof(LoginResult), 0);
		}break;
		case CMD_LOGINOUT:
		{
			Logout *_logout;
			_logout = (Logout*)header;
			std::cout << "�յ�" << "socket = " << pClient->getSock() << " ���CMD_LOGOUT" << " ���ݳ��� = " << header->dataLength << " UserName = " << _logout->userName << std::endl;
			LogoutResult _logoutres;
			send(pClient->getSock(), (char*)&_logoutres, sizeof(LogoutResult), 0);
		}break;
		default:
		{
			std::cout << "�յ���������Ϣ: δ������Ϣ:" << " ���ݳ���:" << header->dataLength << std::endl;
			//header->cmd = CMD_ERROR;
			//send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
		}
		break;
		}
	}
	virtual void OnNetJoin(ClientSocketPtr& pClient)
	{
		_ClientCount++;
	}
	virtual void OnNetRecv(ClientSocketPtr& pClient)
	{
		_recvCount++;
	}
	void _closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. �ر� socket
#ifdef _WIN32
		//����win sock����
		closesocket(_sock);
		//for (int n = _clients.size() - 1; n >= 0; n--)
		//{
		//	closesocket(_clients[n]->getSock());
		//	delete _clients[n];
		//}
		// ���Windows socket����
		WSACleanup();
#else
		close(_sock);
#endif
		//std::cout << "�������" << std::endl;
		getchar();
		//_clients.clear();
		_sock = INVALID_SOCKET;
	}
};

#endif // !EasyTcpServer_hpp_
