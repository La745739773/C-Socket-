#include"EasyTcpServer.hpp"


int main()
{
	EasyTcpServer Server;
	Server.initSocket();
	Server.Bind(nullptr, 4567);
	Server.Listen(5);
	while (Server.isRun())
	{
		Server.OnRun();
	}
	Server._closeSocket();
	cout << "ÈÎÎñ½áÊø" << endl;
	getchar();
	return 0;
}