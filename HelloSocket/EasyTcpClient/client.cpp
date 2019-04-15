#include"EasyTcpClient.hpp"
#include<thread>	
#include<atomic>
#include<mutex>
atomic_int sum = 0;
bool g_bRun = true; 
void cmdThread(/*EasyTcpClient* client*/)
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
			//client->closeSocket();
			g_bRun = false;
			return;
		}
		//else if (0 == strcmp(cmdBuf, "login"))
		//{
		//	Login _login;
		//	strcpy(_login.userName, "Evila");
		//	strcpy(_login.Password, "Evila_Password");
		//	// 5 �������������������
		//	int ret = client->SendData(&_login);
		//	//send(client->_sock, (const char*)&header, header->dataLength, 0);
		//}
		//else if (0 == strcmp(cmdBuf, "logout"))
		//{
		//	Logout _logout;
		//	strcpy(_logout.userName, "Evila");
		//	//5 �������������������
		//	client->SendData(&_logout);
		//}
		else
		{
			cout << "����֧�ֵ�����" << endl;
		}
	}
	return;
}

//�ͻ���socket��������
const int cCount = 5000;
const int tCount = 4;	//�߳�����
//�ͻ���socket����
EasyTcpClient* client[cCount];
mutex m_lock;
void sendThread(int theadId)
{
	int count = cCount / tCount;
	int begin = (theadId-1) * count;
	int end = theadId * count;
	char* ipAdd = new char[100];
	for (int i = begin; i < end; i++)
	{
		client[i] = new EasyTcpClient();
	}
	ipAdd = "127.0.0.1";
	unsigned port = 4567;
	for (int i = begin; i < end; i++)
	{
		client[i]->ConnectServer(ipAdd, port);
		m_lock.lock();
		cout << "Connect:" << i << endl;
		m_lock.unlock();
		//cout << "��<" << i << ">��" << "Sock = " << client[i]->_sock << "��������ip:" << ipAdd << "�˿ں�:" << port << endl;
	}
	std::chrono::milliseconds t(5000);
	std::this_thread::sleep_for(t);
	Login _login;
	strcpy(_login.userName, "Evila");
	strcpy(_login.Password, "Evila_Password");
	while (g_bRun)
	{
		// 5 �������������������
		for (int i = begin; i < end; i++)
		{
			client[i]->SendData(&_login);
			client[i]->onRun();
		}
	}
	for (int i = begin; i < end; i++)
	{
		client[i]->closeSocket();
	}
}

int main()
{
	//����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	//���������߳�
	for (int i = 0; i < tCount; i++)
	{
		std::thread t2(sendThread, i + 1);
		t2.detach();
	}
	while (g_bRun)
	{
		Sleep(10);
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