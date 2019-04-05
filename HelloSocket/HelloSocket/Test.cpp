#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include<WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //windows socket2 32的lib库

int main()
{
	//启动 windows socket 2.x 环境
	WORD versionCode = MAKEWORD(2,2);	//创建一个版本号 
	WSADATA data;
	WSAStartup(versionCode, &data);  //启动Socket网络API的函数
	///

	///
	WSACleanup();
	return 0;
}