#include"EasyTcpClient.hpp"
#include<thread>	

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
	EasyTcpClient client;
	client.initSocket();
	client.ConnectServer("127.0.0.1", 4567);
	//����UI�߳�
	std::thread t1(cmdThread, &client);
	t1.detach();
	while (client.isRun())
	{
		client.onRun();
	}

	client.closeSocket();
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