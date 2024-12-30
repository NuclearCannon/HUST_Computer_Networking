#pragma comment(lib,"ws2_32.lib")
# define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <cstdio>
#include <winsock2.h>
#include <list>
#include <cstring>
#include <iostream>
#include <sstream> 
#include <fstream>
#include <WS2tcpip.h>
using namespace std;

enum class SERVER_RTN
{
	SUCCEED, FAIL
};

//���û�����
char recvBuf[4096];
char sendBuf[4096];

string html_404;
string html_500;
string html_505;



u_long blockMode = 1;	//���÷�����ģʽ
WSADATA wsaData;
SOCKET srvSocket = INVALID_SOCKET;//��������socket
list<SOCKET> sessionList;
string root;


// ��sessionSocket�ж�ȡ�ͻ��˵ĵ�ַ�Ͷ˿ڣ����������
void getClientAddrString(SOCKET sessionSocket, ostream& os)
{
	sockaddr_in clientAddr;
	int addrlen = sizeof(clientAddr);
	if (getpeername(sessionSocket, (struct sockaddr*)&clientAddr, &addrlen) < 0) {
		throw "getpeername error";
	}
	// ��ӡ�Զ�IP��ַ�Ͷ˿ں�
	os << inet_ntoa(clientAddr.sin_addr) << ':' << ntohs(clientAddr.sin_port);
	return;
}

// ���ļ��м������ݵ�std::string
SERVER_RTN loadFileToString(const string& file_path, string& dest)
{
	std::ifstream file(file_path);
	// ����ļ��Ƿ�ɹ���
	if (!file.is_open()) {
		std::cerr << "�޷����ļ�: " << file_path << std::endl;
		return SERVER_RTN::FAIL; // ���ط���ֵ��ʾ����
	}
	std::stringstream buffer;
	buffer << file.rdbuf(); // ʹ��rdbuf()��Ա������ȡ�����ļ�
	file.close();	// �ر��ļ�
	dest = buffer.str();
	return SERVER_RTN::SUCCEED; // �������ʾ�ɹ�
}

// ��Ựsocket�д����ļ������ɹ�ʱ����0��������1
SERVER_RTN sendStream(istream& content_stream, SOCKET sessionSocket)
{
	while (content_stream.good()) {
		content_stream.read(sendBuf, sizeof(sendBuf)); // ��ȡ���ݵ�������  
		std::streamsize bytesRead = content_stream.gcount(); // ��ȡʵ�ʶ�ȡ���ֽ���  
		if (bytesRead > 0) { 
			if (send(sessionSocket, sendBuf, bytesRead, 0) != bytesRead)
			{
				return SERVER_RTN::FAIL;
			}	
		}
		// ����ȡ����ĩβ��������ʱ��ѭ������Ȼ��ֹ  
	}
	return SERVER_RTN::SUCCEED;
}


// ��ʼ��ȫ�ֱ���
SERVER_RTN initialize()  
{
	
	int nRc = WSAStartup(0x0202, &wsaData);
	if (nRc) {
		throw "nRa!=0";
	}
	if (wsaData.wVersion != 0x0202)throw "wsaData.wVersion != 0x0202";
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	// af=AF_INET: ʹ��Internet��ַ
	// type=SOCK_STREAM�������ͣ���֮��Ե������ݱ����ͣ�
	// protocol=0���Զ�ѡ��Э�顣���� 2 �����õ�ֵΪ��IPPROTO_UDP �� IPPROTO_TCP��
	if (srvSocket == INVALID_SOCKET)throw "Socket create fail!";

	//���÷������Ķ˿ں͵�ַ
	int port;//�˿ں�

	printf("���õ�ַ���˿ڣ���Ŀ¼\n�磺127.0.0.1 5050 ./page\n");
	string IP_str;
	cin >> IP_str >> port >> root;
	
	
	if (loadFileToString(root + "/404.html", html_404) == SERVER_RTN::FAIL)return SERVER_RTN::FAIL;
	if (loadFileToString(root + "/500.html", html_500) == SERVER_RTN::FAIL)return SERVER_RTN::FAIL;
	if (loadFileToString(root + "/505.html", html_505) == SERVER_RTN::FAIL)return SERVER_RTN::FAIL;


	sockaddr_in addr;//��������ַ�Ϳͻ��˵�ַ
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(IP_str.c_str());

	if (bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
		throw "Socket bind fail!";
	//����
	if (listen(srvSocket, 5) == SOCKET_ERROR)
		throw "Socket listen fail!";
	// ��srvSocket��Ϊ������ģʽ
	if (ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)
		throw "ioctlsocket() failed with error!";
	return SERVER_RTN::SUCCEED;
}


// ���ڷ���404, 500, 505��html�ļ�
SERVER_RTN sendErrorHtml(int code, SOCKET sessionSocket)
{
	string respond_line;
	const string* content_ptr;
	switch (code)
	{
	case 404: {
		respond_line = "HTTP/1.1 404 Not Found";
		content_ptr = &html_404;
		break;
	}
	case 505: {
		respond_line = "HTTP/1.1 505 HTTP Version Not Supported";
		content_ptr = &html_505;
		break;
	}
	default: {  // ����500
		respond_line = "HTTP/1.1 500 Internal Server Error";
		content_ptr = &html_500;
		break;
	}
	}
	stringstream message;
	message << respond_line << "\r\n";
	message << "Content-Type: text/html; charset=ISO-8859-1\r\n";
	message << "Content-Length: " << content_ptr->size() << "\r\n";
	message << "\r\n";
	message << *content_ptr;
	printf("sending error html...");
	if (sendStream(message, sessionSocket) == SERVER_RTN::FAIL)
	{
		printf("send error html error\n");
		return SERVER_RTN::FAIL;
	}
	printf("sending error html complete\n");
	return SERVER_RTN::SUCCEED;
}



int respond(const string& method, const string& url, const string& http_version, SOCKET sessionSocket)
{

	if (method != "GET")
	{
		// �������ڲ�����
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	if (http_version != "HTTP/1.1")
	{
		// http�汾��֧��
		sendErrorHtml(505, sessionSocket);
		return 505;
	}
	// ��ͼ���ļ�

	string file_path = root + url;
	// ȡ��׺��
	string extension;
	size_t dotPosition = file_path.rfind('.');
	if (dotPosition == std::string::npos) {
		// û���ҵ�'.'��˵����������
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	// ���'.'��·�������һ���ַ���˵����������'.'��β�������ļ���Ŀ¼  
	// ����������Ƿ��ʹ���  
	if (dotPosition == file_path.length() - 1) {
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	std::ifstream file(file_path, std::ios::binary | std::ios::ate); // ���ļ����Զ�����ģʽ������λ���ļ�ĩβ  
	if (!file.is_open()) {
		std::cerr << "�޷����ļ�: " << file_path << std::endl;
		// �ļ����б��д�ʧ�ܣ���404����
		sendErrorHtml(404, sessionSocket);
		return 404;
	}
	int file_size = file.tellg();
	printf("�ɹ�ȡ���ļ�����СΪ%dbytes\n", file_size);
	string content_type;
	extension = file_path.substr(dotPosition + 1);

	if (extension == "html")content_type = "text/html; charset=ISO-8859-1";
	else if (extension == "ico")content_type = "image/icon";
	else if (extension == "png")content_type = "image/png";
	else {
		cerr << "δ֪���ļ����ͣ�" << extension << endl;
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	file.seekg(0, std::ios::beg); // �����ļ���ȡλ�õ��ļ���ͷ  
	// ׼��������Ӧ����
	stringstream header;  // �ַ���
	// �򵥵���Ӧ���ģ�Ϊ�˼��ٴ�����ʡȥ�˺ܶ಻��Ҫ���ײ���
	header << "HTTP/1.1 200 OK\r\n";
	header << "Content-Type: " << content_type << "\r\n";
	header << "Content-Length: " << file_size << "\r\n";  // �ļ���С
	header << "\r\n";
	printf("sending...");
	if (sendStream(header, sessionSocket) == SERVER_RTN::FAIL)
	{
		printf("send string response error");
	}
	if (sendStream(file, sessionSocket) == SERVER_RTN::FAIL)// ����Ӧ���ļ�Ҳ���ͳ�ȥ
	{
		printf("send file error");
	}
	printf("sending complete\n");
	file.close();
	return 200;
}

void loop()
{
	int listLen = -1;
	while (true)
	{
		//���read��������������������Ϊһ�����ϣ�����ռ��ϣ�
		fd_set rfds;
		FD_ZERO(&rfds);
		//���õȴ��ͻ���������
		FD_SET(srvSocket, &rfds);
		for (SOCKET sessionSocket : sessionList)
		{
			//���õȴ��ỰSOKCET�ɽ������ݻ�ɷ�������
			FD_SET(sessionSocket, &rfds);
		}
		//��ʼ�ȴ�
		// select�������ͣ�
		// maxfdp:�����������ļ��������ķ�Χ��Ϊ�����ļ������������ֵ��1
		// fd_set *readfds:Ҫ���ӵĶ��ļ���
		// fd_set *writefds:Ҫ���м��ӵ�д�ļ�����
		// fd_set *errorfds=NULL:���ڼ����쳣����
		// struct timeval* timeout=NULL������Ϊ����״̬��һ��Ҫ�ȵ������ļ�������������ĳ���ļ����������ͱ仯�ŷ���

		// �������ֵ
		int max_val = srvSocket;
		for (SOCKET sessionSocket : sessionList)if (max_val < sessionSocket)max_val = sessionSocket;

		int nTotal = select(max_val + 1, &rfds, NULL, NULL, NULL);

		// ����ֵ��
		// nTotal>0�����ӵ��ļ��������б仯�����ص��Ƕ�Ӧλ��ȻΪ1��fd����
		// -1:����
		// 0;��ʱ


		if (nTotal <= 0) {
			printf("main loop: nTotal=%d: continue", nTotal);
			continue;
		}
		//���ỰSOCKET�Ƿ������ݵ���
		list<SOCKET> newList;
		for (SOCKET sessionSocket : sessionList)
		{
			//����ỰSOCKET�����ݵ���������ܿͻ�������
			if (FD_ISSET(sessionSocket, &rfds)) {

				memset(recvBuf, '\0', sizeof(recvBuf));
				int rtn = recv(sessionSocket, recvBuf, sizeof(recvBuf), 0);
				if (rtn > 0)
				{
					cout << "��ַΪ";
					getClientAddrString(sessionSocket, cout);
					cout << "��sessionSocket�������µı���" << endl;
					printf("-----------------------------------------\n%s\n-----------------------------------------\n", recvBuf);


					// �˴��߼������������=GET /index.html HTTP/1.1�Ͱ�index.html���ػ�ȥ
					// �����������ײ���
					// �������ģ�����������ȫ�洢��recvBuf�У��л�ȡ��һ��
					istringstream iss(recvBuf);
					string method, url, http_version;
					iss >> method >> url >> http_version;  // ����������״��ȡ�����е���������
					int code = respond(method, url, http_version, sessionSocket);
					printf("sessionSocket���񵽵ı����Ѿ���Ӧ��ϣ�״̬��Ϊ%d\n", code);
					newList.push_back(sessionSocket);
				}
				else {
					printf("Client leaving ...\n");
					closesocket(sessionSocket);  //��Ȼclient�뿪�ˣ��͹ر�sessionSocket
				}
			}
			else
			{
				newList.push_back(sessionSocket);
			}
		}
		sessionList.swap(newList);
		//���srvSock�յ��������󣬽��ܿͻ���������
		if (FD_ISSET(srvSocket, &rfds))// ���������������srvSocket�����˱仯
		{
			sockaddr_in clientAddr;//�ͻ��˵�ַ
			clientAddr.sin_family = AF_INET;
			int addrLen = sizeof(clientAddr);
			//�����ỰSOCKET
			SOCKET sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET && ioctlsocket(sessionSocket, FIONBIO, &blockMode) != SOCKET_ERROR)//�ѻỰSOCKET��Ϊ������ģʽ
			{
				cout << "srvSocket�������µ�����������Դ��";
				getClientAddrString(sessionSocket, cout);
				cout << endl;
				sessionList.push_back(sessionSocket);
			}
			else
			{
				throw "accept error";
			}
		}

		int newLen = sessionList.size();
		if (newLen != listLen)
		{
			std::printf("len->%d\n", newLen);
			listLen = newLen;
		}

	}
}

int main()
{
	// ע����������ʹ�ó����˳�ʱ���Թر����е�socket
	if (initialize() == SERVER_RTN::FAIL)return 1;
	loop();
	return 0;
}

