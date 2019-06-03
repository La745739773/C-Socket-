#include"EasyTcpServer.hpp"
#include<thread>
bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		// 3 ������������
		char cmdBuf[128] = {};
		std::cout << "��������: ";
		std::cin >> cmdBuf;
		// 4 ��������
		if (strcmp(cmdBuf, "exit") == 0)
		{
			std::cout << "�˳�cmdThread�߳�" << std::endl;
			g_bRun = false;
			return;
		}
		else
		{
			std::cout << "����֧�ֵ�����" << std::endl;
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
	Server.StartCellServers(2);
	std::thread t1(cmdThread);
	t1.detach();
	while (g_bRun)
	{
		Server.OnRun();
	}
	Server._closeSocket();
	std::cout << "�������" << std::endl;
	getchar();
	return 0;
}