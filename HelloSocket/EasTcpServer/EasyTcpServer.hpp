#ifndef EasyTcpServer_hpp_
#define EasyTcpServer_hpp_
#include "MessageHeadr.hpp"
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
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
		SOCKET _clientSock = INVALID_SOCKET; // ��ʼ����Ч��socket �����洢����Ŀͻ���

		_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &cliendAddrLen);//���ͻ��˽���ʱ ��õ�����ͻ��˵�socket��ַ�ͳ���
		if (INVALID_SOCKET == _clientSock) //���ܵ���Ч����
		{
			cout << "Socket = <" << _sock << " ERROR: ���ܵ���Ч�ͻ���SOCKET..." << endl;
		}
		else
		{
			cout << "��Client���룺" << "socket = " << _clientSock << " IP = " << inet_ntoa(_clientAddr.sin_addr) << endl;  //inet_ntoa ��ip��ַת���ɿɶ����ַ���
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

			for (size_t n = 0; n < g_clinets.size(); n++)
			{
				FD_SET(g_clinets[n], &fdRead);		//��������Ŀͻ��˷���ɶ��б� ��֤recv������
			}

			//nfds��һ������ ��һ������ֵ ��ָfd_set����������socketֵ�ķ�Χ �������� 
			timeval t = { 1,0 }; //select��ѯ��ʱ��ʱ��  windows�µļ�ʱ�� Ŀǰû�м���΢��  0��ʾselect���������ѯû����Ҫ������������
			int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExpect, &t);
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
			for (size_t n = 0; n < fdRead.fd_count; n++)
			{
				if (RecvData(fdRead.fd_array[n]) == -1)//processor�����Ǵ���������߼� recv�ӵ������ݲ�������Ӧ���жϺ������־
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

	//select����ģ��
	//��������
	int RecvData(SOCKET _clientSock)
	{
		char *szRecv = new char[1024];
		//5 ���Ƚ������ݰ�ͷ
		int nlen = recv(_clientSock, szRecv, sizeof(DataHeader), 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
		if (nlen <= 0)
		{
			//�ͻ����˳�
			cout << "�ͻ���:Socket = " << _clientSock << " ���˳����������" << endl;
			return -1;
		}
		DataHeader* header = (DataHeader*)szRecv;
		recv(_clientSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_clientSock, header);
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
				cout << "�յ�" << "socket = " << _clientSock << " ���CMD_LOGIN" << " ���ݳ��� = " << header->dataLength << " UserName = " << _login->userName << " Password = " << _login->Password << endl;
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
				header->cmd = CMD_ERROR;
				header->dataLength = 0;
				send(_clientSock, (char*)&header, sizeof(DataHeader), 0);
			}
			break;
		}
	}

	//���͵�ָ����socket����
	int SendData(SOCKET _clinetSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			//cout << "�������ݸ������" << endl;
			int ret = send(_clinetSock, (const char*)header, header->dataLength, 0);
			return ret;
		}
		return SOCKET_ERROR;
	}

	void SendData2All(DataHeader *header)
	{
		if (isRun() && header)
		{
			//cout << "�������ݸ������" << endl;
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
		//7. �ر� socket
#ifdef _WIN32
		//����win sock����
		closesocket(_sock);
		for (int n = g_clinets.size() - 1; n >= 0; n--)
		{
			closesocket(g_clinets[n]);
		}
		// ���Windows socket����
		WSACleanup();
#else
		close(_sock);
#endif
		cout << "�������" << endl;
		getchar();
		_sock = INVALID_SOCKET;
	}
};

#endif // !EasyTcpServer_hpp_
