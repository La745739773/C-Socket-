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


	//1. ����һ��Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0); //�����������ӵ�ipv4��socket

	if (INVALID_SOCKET == _sock)	//SIocket ÿһ���������ж��Ƿ�ɹ�����
	{
		cout << "ERROR: SOCKET ����ʧ��" << endl;
	}
	else
	{
		cout << "SUCCESS: SOCKET �����ɹ�" << endl;
	}

	//2. ���ӷ����� connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567); //host to net short
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	
	if (SOCKET_ERROR == connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)))	//SIocket ÿһ���������ж��Ƿ�ɹ�����
	{
		cout << "ERROR: SOCKET ����ʧ��" << endl;
	}
	else
	{
		cout << "SUCCESS: SOCKET ���ӳɹ�" << endl;
	}

	while (true)
	{
		// 3 ������������
		char cmdBuf[128] = {};
		cout << "��������: ";
		cin >> cmdBuf;
		// 4 ��������
		if (strcmp(cmdBuf, "exit") == 0)
		{
			break;
		}
		else
		{
			// 5 �������������������
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}
		//�ŵ�ѭ���� �ظ����ܷ������ķ�����Ϣ
		//6. ���ܷ�������Ϣ recv
		char recvBuf[256] = {};
		int nlen = recv(_sock, recvBuf, sizeof(recvBuf), 0); //����ֵ�ǽ������ݵĳ���
		if (nlen > 0)
		{
			cout << recvBuf << endl;
		}
		else
		{
			cout << "ERROR: ���ݴ���ʧ��..." << endl;
		}
	}
	//7. �ر� socket
	closesocket(_sock);
	// ���Windows socket����
	WSACleanup();
	cout << "�������" << endl;
	getchar();
	return 0;
}