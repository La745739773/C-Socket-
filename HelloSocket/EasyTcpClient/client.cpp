#include"EasyTcpClient.hpp"
#include<thread>	
bool g_bRun = true; 
void cmdThread(EasyTcpClient* client)
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
			client->closeSocket();
			g_bRun = false;
			return;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login _login;
			strcpy(_login.userName, "Evila");
			strcpy(_login.Password, "Evila_Password");
			// 5 向服务器发送请求命令
			int ret = client->SendData(&_login);
			//send(client->_sock, (const char*)&header, header->dataLength, 0);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout _logout;
			strcpy(_logout.userName, "Evila");
			//5 向服务器发送请求命令
			client->SendData(&_logout);
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
	const int cCount = 300;
	EasyTcpClient* client = new EasyTcpClient[cCount];
	char* ipAdd = new char[100];
	for (int i = 0; i < cCount; i++)
	{
		client[i].initSocket();
	}
	ipAdd = "127.0.0.1";
	unsigned port = 4567;
	for (int i = 0; i < cCount; i++)
	{
		client[i].ConnectServer(ipAdd, port);
		cout << "Sock = " << client[i]._sock << "正在连接ip:" << ipAdd << "端口号:" << port << endl;
	}

	//启动UI线程
	//std::thread t1(cmdThread, &client);
	//t1.detach();
	Login _login;
	strcpy(_login.userName, "Evila");
	strcpy(_login.Password, "Evila_Password");
	while (g_bRun)
	{
		//client.onRun();
		// 5 向服务器发送请求命令
		for (int i = 0; i < cCount; i++)
		{
			client[i].SendData(&_login);
			if (client[i].onRun() == false)
			{
				delete &client[i];
			}
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






//// 3 输入请求命令
//char cmdBuf[128] = {};
//cout << "输入命令: ";
//cin >> cmdBuf;
//// 4 处理请求
//if (strcmp(cmdBuf, "exit") == 0)
//{
//	break;
//}
//else if (0 == strcmp(cmdBuf,"login"))
//{
//	Login _login;
//	strcpy(_login.userName, "Evila");
//	strcpy(_login.Password, "Evila_Password");
//	// 5 向服务器发送请求命令
//	send(_sock, (const char*)&_login, sizeof(Login), 0);

//	//6. 接受服务器信息 recv
//	LoginResult _lgRes;
//	recv(_sock, (char*)&_lgRes, sizeof(LoginResult), 0);
//	cout<<"LoginResult: " << _lgRes.result << endl;
//}
//else if (0 == strcmp(cmdBuf, "logout"))
//{
//	Logout _logout;
//	strcpy(_logout.userName, "Evila");
//	// 5 向服务器发送请求命令 
//	send(_sock, (const char*)&_logout, sizeof(Logout), 0);

//	//6. 接受服务器信息 recv
//	LogoutResult _lgRes;
//	//返回数据
//	recv(_sock, (char*)&_lgRes, sizeof(LogoutResult), 0);
//	cout << "LogoutResult: " << _lgRes.result << endl;
//}
//else
//{
//	cout << "不支持的命令，请重新输入。" << endl;
//}