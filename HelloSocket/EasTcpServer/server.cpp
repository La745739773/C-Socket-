#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<string>
using namespace std;
#pragma comment(lib,"ws2_32.lib") //windows socket2 32��lib��

int main()
{
	//���� windows socket 2.x ����
	WORD versionCode = MAKEWORD(2, 2);	//����һ���汾�� 
	WSADATA data;
	WSAStartup(versionCode, &data);  //����Socket����API�ĺ���
									 ///


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
	//	3. ��������˿� listen

	if (listen(_sock, 5) == SOCKET_ERROR)//�ڶ������� backbag ���������������
	{
		cout << "ERROR: �������ڽ��ܿͻ������ӵ�����˿�ʧ��..." << endl;
	}
	else
	{
		cout << "SUCCESS: �����˿ڳɹ�..." << endl;
	}

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
		cout << "��Client���룺"<< "socket = "<< _clientSock <<"IP = " << inet_ntoa(_clientAddr.sin_addr) << endl;  //inet_ntoa ��ip��ַת���ɿɶ����ַ���
	}
	char _recvBuf[128] = {};
	while (true)
	{
		//5 ���ܿͻ�����������
		int nlen = recv(_clientSock, _recvBuf, 128, 0); //���ܿͻ��˵����� ��һ������Ӧ���ǿͻ��˵�socket����
		if (nlen <= 0)
		{
			//�ͻ����˳�
			cout << "�ͻ������˳����������" << endl;
			break;
		}
		cout << "�յ����" << _recvBuf << endl;
		//6 ��������  ����ֻ�Ǽ򵥵��ַ����ж�
		if (0 == strcmp(_recvBuf, "getName"))
		{
			//	7. ��ͻ��˷���һ������ send
			char msgBuf[] = "Evila";
			send(_clientSock, msgBuf, strlen(msgBuf) + 1, 0); //+1��Ϊ�˰�\0���ȥ
		}
		else if (0 == strcmp(_recvBuf, "getAge"))
		{
			//	7. ��ͻ��˷���һ������ send
			char msgBuf[] = "80";
			send(_clientSock, msgBuf, strlen(msgBuf) + 1, 0); //+1��Ϊ�˰�\0���ȥ
		}
		else
		{
			//	7. ��ͻ��˷���һ������ send
			char msgBuf[] = "Hello, I'm Server";
			send(_clientSock, msgBuf, strlen(msgBuf) + 1, 0); //+1��Ϊ�˰�\0���ȥ
		}
	}
	//	8. �ر�socket
	closesocket(_sock);
	// ���Windows socket����
	WSACleanup();
	cout << "�������" << endl;
	getchar();
	return 0;
}