#include"EasyTcpClient.hpp"
#include"CELLTimestamp.hpp"
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
		else
		{
			cout << "����֧�ֵ�����" << endl;
		}
	}
	return;
}

//�ͻ���socket��������
const int cCount = 10;
const int tCount = 4;	//�߳�����
//�ͻ���socket����
EasyTcpClient* client[cCount];
mutex m_lock;
std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;	//�߳�׼������������ԭ�Ӳ���ʵ�ֵȴ�����
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
	//ipAdd = "192.168.1.102"; //�ʼǱ�ip
	//ipAdd = "114.212.120.196";

	ipAdd = "192.168.1.101";   //̨ʽ��ip
	//ipAdd = "127.0.0.1";
	unsigned port = 4567;
	for (int i = begin; i < end; i++)
	{
		client[i]->ConnectServer(ipAdd, port);
		//m_lock.lock();
		//cout <<"thread<"<< theadId <<">,"<< "Connect:" << i << endl;
		//m_lock.unlock();
		//cout << "��<" << i << ">��" << "Sock = " << client[i]->_sock << "��������ip:" << ipAdd << "�˿ں�:" << port << endl;
	}
	//std::chrono::milliseconds t(5000);
	//std::this_thread::sleep_for(t);
	readyCount++;
	while (readyCount < tCount)	
	{
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}
	m_lock.lock();
	cout << "thread<" << theadId << ">," << "Connect<begin=" << begin << ",end=" << end << ">" << endl;
	m_lock.unlock();
	Login _login[10];
	for (int i = 0; i < 2; i++)
	{
		strcpy(_login[i].userName, "Evila");
		strcpy(_login[i].Password, "Evila_Password");
	}
	CELLTimestamp tTime;
	const int nlen = sizeof(_login);
	while (g_bRun)
	{
		// 5 �������������������
		for (int i = begin; i < end; i++)
		{
			if (SOCKET_ERROR != client[i]->SendData(_login, nlen))
			{
				sendCount++;
			}
			//if (tTime.getElapsedSecond() >= 1.0)
			//{
			//	if (SOCKET_ERROR != client[i]->SendData(_login, nlen))
			//	{
			//		sendCount++;
			//	}
			//	tTime.update();
			//}
			client[i]->onRun();
		}
	}
	for (int i = begin; i < end; i++)
	{
		client[i]->closeSocket();
		delete client[i];
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
	CELLTimestamp tTime;
	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		//cout << "test" << endl;
		//cout << "thread<" << tCount << ">" << ",clients<" << cCount << ">,time<" << t << ">,send<" << sendCount << ">" << endl;
		if (t >= 1.0)
		{
			cout << "thread<"<<tCount<<">"<<",clients<"<< cCount <<">,time<"<<t<<">,send<"<<sendCount<<">"<< endl;
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	delete[] client;
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