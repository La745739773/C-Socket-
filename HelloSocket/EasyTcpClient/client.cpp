#include"EasyTcpClient.hpp"
#include<thread>	
bool g_bRun = true; 
void cmdThread(EasyTcpClient* client)
{
	while (true)
	{
		// 3 ������������
		char cmdBuf[128] = {};
		cout << "��������: ";
		cin >> cmdBuf;
		// 4 ��������
		if (strcmp(cmdBuf, "exit") == 0)
		{
			cout << "�˳�cmdThread�߳�" << endl;
			client->closeSocket();
			g_bRun = false;
			return;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login _login;
			strcpy(_login.userName, "Evila");
			strcpy(_login.Password, "Evila_Password");
			// 5 �������������������
			int ret = client->SendData(&_login);
			//send(client->_sock, (const char*)&header, header->dataLength, 0);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout _logout;
			strcpy(_logout.userName, "Evila");
			//5 �������������������
			client->SendData(&_logout);
		}
		else
		{
			cout << "����֧�ֵ�����" << endl;
		}
	}
	return;
}


int main()
{
	const int cCount = FD_SETSIZE - 1;
	EasyTcpClient* client = new EasyTcpClient[cCount];
	for (int i = 0; i < cCount; i++)
	{
		char* ipAdd = new char[100];
		ipAdd = "127.0.0.1";
		unsigned port = 4567;
		client[i].initSocket();;
		client[i].ConnectServer(ipAdd, port);
		cout << "Sock = " << client[i]._sock << "��������ip:" << ipAdd << "�˿ں�:" << port << endl;
	}

	//����UI�߳�
	//std::thread t1(cmdThread, &client);
	//t1.detach();
	Login _login;
	strcpy(_login.userName, "Evila");
	strcpy(_login.Password, "Evila_Password");
	while (g_bRun)
	{
		//client.onRun();
		// 5 �������������������
		for (int i = 0; i < cCount; i++)
		{
			if (client[i].onRun() == false)
			{
				delete &client[i];
			}
			client[i].SendData(&_login);
		}

	}
	for (int i = 0; i < cCount; i++)
	{
		client[i].closeSocket();
	}
	delete client;
	getchar();
	return 0;
}






//// 3 ������������
//char cmdBuf[128] = {};
//cout << "��������: ";
//cin >> cmdBuf;
//// 4 ��������
//if (strcmp(cmdBuf, "exit") == 0)
//{
//	break;
//}
//else if (0 == strcmp(cmdBuf,"login"))
//{
//	Login _login;
//	strcpy(_login.userName, "Evila");
//	strcpy(_login.Password, "Evila_Password");
//	// 5 �������������������
//	send(_sock, (const char*)&_login, sizeof(Login), 0);

//	//6. ���ܷ�������Ϣ recv
//	LoginResult _lgRes;
//	recv(_sock, (char*)&_lgRes, sizeof(LoginResult), 0);
//	cout<<"LoginResult: " << _lgRes.result << endl;
//}
//else if (0 == strcmp(cmdBuf, "logout"))
//{
//	Logout _logout;
//	strcpy(_logout.userName, "Evila");
//	// 5 ������������������� 
//	send(_sock, (const char*)&_logout, sizeof(Logout), 0);

//	//6. ���ܷ�������Ϣ recv
//	LogoutResult _lgRes;
//	//��������
//	recv(_sock, (char*)&_lgRes, sizeof(LogoutResult), 0);
//	cout << "LogoutResult: " << _lgRes.result << endl;
//}
//else
//{
//	cout << "��֧�ֵ�������������롣" << endl;
//}