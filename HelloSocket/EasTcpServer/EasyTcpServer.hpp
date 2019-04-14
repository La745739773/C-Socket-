#ifndef EasyTcpServer_hpp_
#define EasyTcpServer_hpp_
#include "MessageHeadr.hpp"
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#ifndef FD_SETSIZE
	#define FD_SETSIZE      1024
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
using namespace std;
#ifndef RECV_BUFF_SIZE
	#define RECV_BUFF_SIZE 10240
#endif

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
private:
	SOCKET _sockfd;  //fd_set file desc set
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	int _lastPos;
};
class EasyTcpServer
{
public:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;	//��ָ�� ��ֹ��ջ ��Ϊnew�Ķ������ڶ��ڴ� ����ջ�ڴ�
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
		//����Windows sock 2.x����
		WORD versionCode = MAKEWORD(2, 2);	//����һ���汾�� 
		WSADATA data;
		WSAStartup(versionCode, &data);  //����Socket����API�ĺ���
#endif
										 //1. ����һ��Socket
		if (_sock != INVALID_SOCKET)
		{
			cout << "<socket=>" << _sock << " �ر����������Ӳ����½�������" << endl;
			_closeSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //�����������ӵ�ipv4��socket TCP����
		if (INVALID_SOCKET == _sock)	//SIocket ÿһ���������ж��Ƿ�ɹ�����
		{
			cout << "Socket = <" << _sock << ">" << " ERROR: SOCKET ����ʧ��" << endl;
		}
		else
		{
			cout << "Socket = <" << _sock << ">" << " SUCCESS: SOCKET �����ɹ�" << endl;
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
			cout << "Socket = <" << _sock << ">" << " ERROR: �����ڽ��ܿͻ������ӵ�����˿�<"<<port<<">ʧ��..." << endl;
			return -1;
		}
		else
		{
			cout << "Socket = <" << _sock << ">" << " SUCCESS: �󶨶˿ڳɹ�..." << endl;
		}
		return 0;
	}
	//�����˿�
	int Listen(int _maxNumber)
	{
		if (listen(_sock, _maxNumber) == SOCKET_ERROR)//�ڶ������� backbag ���������������
		{
			cout << "Socket = <"<<_sock<<">"<<" ERROR: �������ڽ��ܿͻ������ӵ�����˿�ʧ��..." << endl;
			return SOCKET_ERROR;
		}
		else
		{
			cout << "Socket = <" << _sock << ">" << " SUCCESS: �����˿ڳɹ�..." << endl;
			return 1;
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
			cout << "Socket = <" << _sock << " ERROR: ���ܵ���Ч�ͻ���SOCKET..." << endl;
		}
		else
		{
			cout << "��Client���룺" << "socket = " << clientSock << " IP = " << inet_ntoa(_clientAddr.sin_addr) << endl;  //inet_ntoa ��ip��ַת���ɿɶ����ַ���
			NewUserJoin userJoin;
			userJoin.cmd = CMD_NEWUSERJOIN;
			userJoin.sockId = clientSock;
			//SendData2All(&userJoin);
			_clients.push_back(new ClientSocket(clientSock));
		}
		return clientSock;
	}
	int _nCount = 0;
	//����������Ϣ
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExpect;

			FD_ZERO(&fdRead);		//���fd���ϵ�����
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExpect);
			//�����Ĺ����� ������˵�_sock �ŵ�fdRead��������� 
			//��socket��listen״̬������Ѿ�����һ�������������socket�ᱻ���Ϊ�ɶ�������һ��accept��ȷ���������������
			//����������socket���ɶ�����ζ�Ŷ����е������ʺ϶���������recv�󲻻�������
			FD_SET(_sock, &fdRead);  //������˵�socket����ɶ��б�ȷ��accept������
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExpect);
			SOCKET maxSock = _sock;
			for (size_t n = 0; n < _clients.size(); n++)
			{
				FD_SET(_clients[n]->getSock(), &fdRead);		//��������Ŀͻ��˷���ɶ��б� ��֤recv������
				if (maxSock < _clients[n]->getSock())
				{
					maxSock = _clients[n]->getSock();
				}
			}

			//nfds��һ������ ��һ������ֵ ��ָfd_set����������socketֵ�ķ�Χ �������� 
			timeval t = { 0,0 }; //select��ѯ��ʱ��ʱ��  windows�µļ�ʱ�� Ŀǰû�м���΢��  0��ʾselect���������ѯû����Ҫ������������
			int ret = select(maxSock, &fdRead, &fdWrite, &fdExpect, &t);
			//cout << "select result = "<<ret << "count = "<< _nCount++ << endl;
			if (ret < 0)
			{
				cout << "select�������" << endl;
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))	//�ж�_sock�Ƿ���fdRead�У� ����� �����пͻ�����������
			{
				FD_CLR(_sock, &fdRead);
				Accept();
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
							 delete (*it);
							 _clients.erase(it);
						}
					}
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

	//select����ģ��
	//��������
	char _szRecv[RECV_BUFF_SIZE] = {};
	int RecvData(ClientSocket* pClient)
	{
		//5 ���Ƚ������ݰ�ͷ
		int nlen = recv(pClient->getSock(), _szRecv, RECV_BUFF_SIZE, 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
		if (nlen <= 0)
		{
			//�ͻ����˳�
			cout << "�ͻ���:Socket = " << pClient->getSock() << " ���˳����������" << endl;
			return -1;
		}
		//�ڴ濽�� ����ȡ�����ݿ�������Ϣ��������
		memcpy(pClient->getMsgbuff() + pClient->getLstpos(), _szRecv, nlen);
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
				OnNetMsg(pClient->getSock(),header);
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
	virtual void OnNetMsg(SOCKET _clientSock, DataHeader *header)
	{
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
				Login* _login;
				_login = (Login*)header;
				//cout << "�յ�" << "socket = " << _clientSock << " ���CMD_LOGIN" << " ���ݳ��� = " << header->dataLength << " UserName = " << _login->userName << " Password = " << _login->Password << endl;
				//�������ж��û��������Ƿ���ȷ�Ĺ���
				LoginResult _loginres;
				send(_clientSock, (char*)&_loginres, sizeof(LoginResult), 0);
			}break;
			case CMD_LOGINOUT:
			{
				Logout *_logout;
				_logout = (Logout*)header;
				cout << "�յ�" << "socket = " << _clientSock << " ���CMD_LOGOUT" << " ���ݳ��� = " << header->dataLength << " UserName = " << _logout->userName << endl;
				LogoutResult _logoutres;
				send(_clientSock, (char*)&_logoutres, sizeof(LogoutResult), 0);
			}break;
			default:
			{
				cout << "�յ���������Ϣ: δ������Ϣ:" << " ���ݳ���:" << header->dataLength << endl;
				//header->cmd = CMD_ERROR;
				//send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
			}
			break;
		}
	}

	//���͵�ָ����socket����
	int SendData(SOCKET _clientsock, DataHeader* header)
	{
		if (isRun() && header)
		{
			//cout << "�������ݸ������" << endl;
			int ret = send(_clientsock, (const char*)header, header->dataLength, 0);
			return ret;
		}
		return SOCKET_ERROR;
	}

	void SendData2All(DataHeader *header)
	{
		if (isRun() && header)
		{
			//cout << "�������ݸ������" << endl;
			for (int n = _clients.size() - 1; n >= 0; n--)
			{
				NewUserJoin userJoin;
				userJoin.cmd = CMD_NEWUSERJOIN;
				userJoin.sockId = _clients[n]->getSock();
				//send(_clients[n], (const char*)&userJoin, userJoin.dataLength, 0);
				SendData(_clients[n]->getSock(), header);
			}
		}
		return;
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
			delete _clients[n];
		}
		// ���Windows socket����
		WSACleanup();
#else
		for (int n = _clients.size() - 1; n >= 0; n--)
		{
			close(_clients[n]->getSock());
			delete _clients[n];
		}
#endif
		cout << "�������" << endl;
		getchar();
		_clients.clear();
		_sock = INVALID_SOCKET;
	}
};

#endif // !EasyTcpServer_hpp_
