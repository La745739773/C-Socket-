#include"Allocator.h"
#include"EasyTcpServer.hpp"
#include<thread>

bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		// 3 输入请求命令
		char cmdBuf[128] = {};
		std::cout << "输入命令: ";
		std::cin >> cmdBuf;
		// 4 处理请求
		if (strcmp(cmdBuf, "exit") == 0)
		{
			std::cout << "退出cmdThread线程" << std::endl;
			g_bRun = false;
			return;
		}
		else
		{
			std::cout << "不受支持的命令" << std::endl;
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
	Server.StartCellServers(4);
	std::thread t1(cmdThread);
	t1.detach();
	while (g_bRun)
	{
		Server.OnRun();
	}
	Server._closeSocket();
	std::cout << "任务结束" << std::endl;
	getchar();
	return 0;
}