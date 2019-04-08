#ifndef EasyTcpClient_hpp_
#define EasyTcpClient_hpp_
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
	//���������� ֱ�ӵĽ���C++�л������virtual������������Ϊ�˷�ֹ�ڴ�й©��
	//�����˵��������������������ڴ�ռ䣬���������������ж���Щ�ڴ�ռ�����ͷš�
	//��������в��õ��Ƿ���������������ɾ������ָ��ָ������������ʱ�Ͳ��ᴥ����̬�󶨣�
	//���ֻ����û������������������������������������������ô����������£�������������Ŀռ�͵ò����ͷŴӶ������ڴ�й©��
	//���ԣ�Ϊ�˷�ֹ��������ķ�����C++�л������������Ӧ����virtual������������
	virtual ~EasyTcpClient()  
	{
		closeSocket();
	}
	//��ʼ��socket
	int initSocket()
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
			cout << "<socket=>"<<_sock<<" �ر����������Ӳ����½�������" << endl;
			closeSocket();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //�����������ӵ�ipv4��socket TCP����
		if (INVALID_SOCKET == _sock)	//SIocket ÿһ���������ж��Ƿ�ɹ�����
		{
			cout << "ERROR: SOCKET ����ʧ��" << endl;
		}
		else
		{
			cout << "SUCCESS: SOCKET �����ɹ�" << endl;
		}
		return 0;
	}
	//���ӷ�����
	int ConnectServer(char* ip,unsigned short port)
	{
		if (_sock == INVALID_SOCKET)
		{
			initSocket();
		}
		//2. ���ӷ����� connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port); //host to net short
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))	//SIocket ÿһ���������ж��Ƿ�ɹ�����
		{
			cout << "ERROR: SOCKET ����ʧ��" << endl;
			getchar();
			return SOCKET_ERROR;
		}
		else
		{
			cout << "SUCCESS: SOCKET ���ӳɹ�" << endl;
			return 0;
		}
	}
	//�ر�socket
	void closeSocket()
	{
		if (_sock == INVALID_SOCKET)
			return;
		//7. �ر� socket
#ifdef _WIN32
		//����win sock����
		closesocket(_sock);
		// ���Windows socket����
		WSACleanup();
#else
		close(_sock);
#endif
		cout << "�������" << endl;
		getchar();
		_sock = INVALID_SOCKET;
	}

	//��ѯselect������Ϣ
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
				cout << "<socket = " << _sock << ">" << " select�������" << endl;
				return false;
			}
			if (FD_ISSET(_sock, &fdReads))	//���_sock��fdRead���棬�����н������ݵȴ�����
			{
				FD_CLR(_sock, &fdReads);

				if (RecvData() == -1)
				{
					cout << "<socket = " << _sock << ">" << " select�������2" << endl;
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
	//��������  ����ճ������ְ�
	int RecvData()
	{
		char *szRecv = new char[1024];
		//5 ���Ƚ������ݰ�ͷ
		int nlen = recv(_sock, szRecv, sizeof(DataHeader), 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
		if (nlen <= 0)
		{
			//�ͻ����˳�
			cout << "�ͻ���:Socket = " << _sock << " ��������Ͽ����ӣ��������" << endl;
			return -1;
		}
		DataHeader* header = (DataHeader*)szRecv;
		recv(_sock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return 0;
	}
	//��Ӧ������Ϣ
	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
			case CMD_NEWUSERJOIN:
			{
				NewUserJoin* _userJoin = (NewUserJoin*)header;
				cout << "�յ���������Ϣ: CMD_NEWUSERJOIN: SocketId = " << _userJoin->sockId << " ���ݳ���:" << header->dataLength << endl;
			}break;
			case CMD_LOGIN_RESULT:
			{
				LoginResult* _lgRes = (LoginResult*)header;
				cout << "�յ���������Ϣ: CMD_LOGIN_RESULT: ��½״̬" << _lgRes->result << endl;
			}break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult* _lgRes = (LogoutResult*)header;
				cout << "�յ���������Ϣ: CMD_LOGIN_RESULT: �ǳ�״̬" << _lgRes->result << endl;
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
			//cout << "�������ݸ������" << endl;
			int ret = send(_sock, (const char*)header, header->dataLength, 0);
			return ret;
		}
		return SOCKET_ERROR;
	}
private:

};



#endif