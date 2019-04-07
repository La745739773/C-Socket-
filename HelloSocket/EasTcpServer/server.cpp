#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<string>
#include<vector>
using namespace std;
#pragma comment(lib,"ws2_32.lib") //windows socket2 32��lib��

//һ��Ҫ��֤����˺Ϳͻ��ˣ�����ϵͳ���� ���ݽṹ�ֽ�˳��ʹ�С��֤һ�� 
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
	CMD_ERROR,
	CMD_NEWUSERJOIN,
};
//��Ϣͷ
struct DataHeader
{
	short dataLength;    //���ݳ��� 32767�ֽ�
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
struct NewUserJoin :public DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEWUSERJOIN;
		sockId = 0;
	}
	int sockId;
};


vector<SOCKET> g_clinets;

int processor(SOCKET _clientSock)
{
	char *szRecv = new char[1024];
	//5 ���Ƚ������ݰ�ͷ
	int nlen = recv(_clientSock, szRecv, sizeof(DataHeader), 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
	if (nlen <= 0)
	{
		//�ͻ����˳�
		cout << "�ͻ���:Socket = "<<_clientSock<<" ���˳����������" << endl;
		return -1;
	}
	DataHeader* header = (DataHeader*)szRecv;
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		Login* _login;
		recv(_clientSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		_login = (Login*)szRecv;
		cout << "�յ�"<< "socket = " << _clientSock <<" ���CMD_LOGIN" << " ���ݳ��� = " << header->dataLength << " UserName = " << _login->userName << " Password = " << _login->Password << endl;
		//�������ж��û��������Ƿ���ȷ�Ĺ���
		LoginResult _loginres;
		send(_clientSock, (char*)&_loginres, sizeof(LoginResult), 0);
	}break;
	case CMD_LOGINOUT:
	{
		Logout *_logout;
		recv(_clientSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		_logout = (Logout*)szRecv;
		cout << "�յ�"<< "socket = " << _clientSock <<" ���CMD_LOGOUT" << " ���ݳ��� = " << header->dataLength << " UserName = " << _logout->userName << endl;
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
	return 0;
}

int main()
{
	//���� windows socket 2.x ����
	WORD versionCode = MAKEWORD(2, 2);	//����һ���汾�� 
	WSADATA data;
	WSAStartup(versionCode, &data);  //����Socket����API�ĺ���

	//(1) ��Socket API�������׵�TCP�����

	//	1. ����һ��Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // ipv4 �����ֽ����� tcpЭ��

	//	2. �󶨽��ܿͻ������ӵĶ˿� bind

	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567); //�˿ں� host to net  sockaddr_in��port��USHORT���� ������port�� unsigend short���� �����ҪHtons����ת��
	//_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //�������󶨵�IP��ַ  127.0.0.1�Ǳ��ص�ַ
	_sin.sin_addr.S_un.S_addr = INADDR_ANY; //���޶����ʸ÷���˵�IP
	if (bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)  //sockaddr �����ڱ���  
	{
		cout << "ERROR: �����ڽ��ܿͻ������ӵ�����˿�ʧ��..." << endl;
	}
	else
	{
		cout << "SUCCESS: �󶨶˿ڳɹ�..." << endl;
	}
	cout << "Socket Id: " << _sock << endl;
	//	3. ��������˿� listen

	if (listen(_sock, 5) == SOCKET_ERROR)//�ڶ������� backbag ���������������
	{
		cout << "ERROR: �������ڽ��ܿͻ������ӵ�����˿�ʧ��..." << endl;
	}
	else
	{
		cout << "SUCCESS: �����˿ڳɹ�..." << endl;
	}
	char _recvBuf[128] = {};
	while (true)
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
		timeval t = {1,0}; //select��ѯ��ʱ��ʱ��  windows�µļ�ʱ�� Ŀǰû�м���΢��  0��ʾselect���������ѯû����Ҫ������������
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExpect, &t);
		if (ret < 0)
		{
			cout << "select�������" << endl;
			break;
		}
		if (FD_ISSET(_sock, &fdRead))	//�ж�_sock�Ƿ���fdRead�У� ����� �����пͻ�����������
		{
			FD_CLR(_sock, &fdRead); 
			//	4. �ȴ����ܿͻ������� accept
			sockaddr_in _clientAddr = {};
			int cliendAddrLen = sizeof(_clientAddr);
			SOCKET _clientSock = INVALID_SOCKET; // ��ʼ����Ч��socket �����洢����Ŀͻ���

			_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &cliendAddrLen);//���ͻ��˽���ʱ ��õ�����ͻ��˵�socket��ַ�ͳ���
			if (INVALID_SOCKET == _clientSock) //���ܵ���Ч����
			{
				cout << "ERROR: ���ܵ���Ч�ͻ���SOCKET..." << endl;
			}
			else
			{
				cout << "��Client���룺" << "socket = " << _clientSock << " IP = " << inet_ntoa(_clientAddr.sin_addr) << endl;  //inet_ntoa ��ip��ַת���ɿɶ����ַ���
			}
			for (int n = g_clinets.size() - 1; n >= 0; n--)
			{	
				NewUserJoin userJoin;
				userJoin.cmd = CMD_NEWUSERJOIN;
				userJoin.sockId = _clientSock;
				send(g_clinets[n], (const char*)&userJoin, userJoin.dataLength, 0);
			}
			g_clinets.push_back(_clientSock);
		}

		for (size_t n = 0; n < fdRead.fd_count; n++)
		{
			if (processor(fdRead.fd_array[n]) == -1)//processor�����Ǵ���������߼� recv�ӵ������ݲ�������Ӧ���жϺ������־
			{
				auto it = find(g_clinets.begin(), g_clinets.end(), fdRead.fd_array[n]);
				if(it != g_clinets.end())
					g_clinets.erase(it);
			}
		}	
		//cout << "�����socket = "<<_sock<<" ���������У���������ҵ��" << endl;
	}
	//	����socket
	for (size_t n = 0; n < g_clinets.size(); n++)
	{
		closesocket(g_clinets[n]);
	}
	//	8. �ر�socket
	closesocket(_sock);
	// ���Windows socket����
	WSACleanup();
	cout << "�������" << endl;
	getchar();
	return 0;
}