#include"EasyTcpServer.hpp"
#include<thread>
bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		// 3 输入请求命令
		char cmdBuf[128] = {};
		cout << "输入命令: ";
		cin >> cmdBuf;
		// 4 处理请求
		if (strcmp(cmdBuf, "exit") == 0)
		{
			cout << "退出cmdThread线程" << endl;
			g_bRun = false;
			return;
		}
		else
		{
			cout << "不受支持的命令" << endl;
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
	cout << "任务结束" << endl;
	getchar();
	return 0;
}