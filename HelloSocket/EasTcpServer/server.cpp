#include"EasyTcpServer.hpp"
#include<thread>
bool g_bRun = true;
void cmdThread()
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
int main()
{
	EasyTcpServer Server;
	Server.initSocket();
	Server.Bind(nullptr, 4567);
	Server.Listen(5);
	std::thread t1(cmdThread);
	t1.detach();
	while (g_bRun)
	{
		Server.OnRun();
	}
	Server._closeSocket();
	cout << "�������" << endl;
	getchar();
	return 0;
}