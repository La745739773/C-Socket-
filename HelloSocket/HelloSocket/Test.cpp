#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include<WinSock2.h>

#pragma comment(lib,"ws2_32.lib") //windows socket2 32��lib��

int main()
{
	//���� windows socket 2.x ����
	WORD versionCode = MAKEWORD(2,2);	//����һ���汾�� 
	WSADATA data;
	WSAStartup(versionCode, &data);  //����Socket����API�ĺ���
	///

	///
	WSACleanup();
	return 0;
}