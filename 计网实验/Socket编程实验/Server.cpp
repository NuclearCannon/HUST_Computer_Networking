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

//设置缓冲区
char recvBuf[4096];
char sendBuf[4096];

string html_404;
string html_500;
string html_505;



u_long blockMode = 1;	//启用非阻塞模式
WSADATA wsaData;
SOCKET srvSocket = INVALID_SOCKET;//声明监听socket
list<SOCKET> sessionList;
string root;


// 从sessionSocket中读取客户端的地址和端口，输出到流中
void getClientAddrString(SOCKET sessionSocket, ostream& os)
{
	sockaddr_in clientAddr;
	int addrlen = sizeof(clientAddr);
	if (getpeername(sessionSocket, (struct sockaddr*)&clientAddr, &addrlen) < 0) {
		throw "getpeername error";
	}
	// 打印对端IP地址和端口号
	os << inet_ntoa(clientAddr.sin_addr) << ':' << ntohs(clientAddr.sin_port);
	return;
}

// 从文件中加载内容到std::string
SERVER_RTN loadFileToString(const string& file_path, string& dest)
{
	std::ifstream file(file_path);
	// 检查文件是否成功打开
	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << file_path << std::endl;
		return SERVER_RTN::FAIL; // 返回非零值表示错误
	}
	std::stringstream buffer;
	buffer << file.rdbuf(); // 使用rdbuf()成员函数读取整个文件
	file.close();	// 关闭文件
	dest = buffer.str();
	return SERVER_RTN::SUCCEED; // 返回零表示成功
}

// 向会话socket中传输文件流，成功时返回0，出错返回1
SERVER_RTN sendStream(istream& content_stream, SOCKET sessionSocket)
{
	while (content_stream.good()) {
		content_stream.read(sendBuf, sizeof(sendBuf)); // 读取数据到缓冲区  
		std::streamsize bytesRead = content_stream.gcount(); // 获取实际读取的字节数  
		if (bytesRead > 0) { 
			if (send(sessionSocket, sendBuf, bytesRead, 0) != bytesRead)
			{
				return SERVER_RTN::FAIL;
			}	
		}
		// 当读取到流末尾或发生错误时，循环会自然终止  
	}
	return SERVER_RTN::SUCCEED;
}


// 初始化全局变量
SERVER_RTN initialize()  
{
	
	int nRc = WSAStartup(0x0202, &wsaData);
	if (nRc) {
		throw "nRa!=0";
	}
	if (wsaData.wVersion != 0x0202)throw "wsaData.wVersion != 0x0202";
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	// af=AF_INET: 使用Internet地址
	// type=SOCK_STREAM（流类型，与之相对的是数据报类型）
	// protocol=0：自动选择协议。另外 2 个常用的值为：IPPROTO_UDP 和 IPPROTO_TCP。
	if (srvSocket == INVALID_SOCKET)throw "Socket create fail!";

	//设置服务器的端口和地址
	int port;//端口号

	printf("配置地址，端口，根目录\n如：127.0.0.1 5050 ./page\n");
	string IP_str;
	cin >> IP_str >> port >> root;
	
	
	if (loadFileToString(root + "/404.html", html_404) == SERVER_RTN::FAIL)return SERVER_RTN::FAIL;
	if (loadFileToString(root + "/500.html", html_500) == SERVER_RTN::FAIL)return SERVER_RTN::FAIL;
	if (loadFileToString(root + "/505.html", html_505) == SERVER_RTN::FAIL)return SERVER_RTN::FAIL;


	sockaddr_in addr;//服务器地址和客户端地址
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(IP_str.c_str());

	if (bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
		throw "Socket bind fail!";
	//监听
	if (listen(srvSocket, 5) == SOCKET_ERROR)
		throw "Socket listen fail!";
	// 将srvSocket设为非阻塞模式
	if (ioctlsocket(srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)
		throw "ioctlsocket() failed with error!";
	return SERVER_RTN::SUCCEED;
}


// 用于发送404, 500, 505的html文件
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
	default: {  // 包括500
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
		// 服务器内部错误
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	if (http_version != "HTTP/1.1")
	{
		// http版本不支持
		sendErrorHtml(505, sessionSocket);
		return 505;
	}
	// 试图打开文件

	string file_path = root + url;
	// 取后缀名
	string extension;
	size_t dotPosition = file_path.rfind('.');
	if (dotPosition == std::string::npos) {
		// 没有找到'.'，说明发生错误
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	// 如果'.'是路径的最后一个字符，说明可能是以'.'结尾的隐藏文件或目录  
	// 这种情况还是发送错误  
	if (dotPosition == file_path.length() - 1) {
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	std::ifstream file(file_path, std::ios::binary | std::ios::ate); // 打开文件，以二进制模式，并定位到文件末尾  
	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << file_path << std::endl;
		// 文件在列表中打开失败，按404处理
		sendErrorHtml(404, sessionSocket);
		return 404;
	}
	int file_size = file.tellg();
	printf("成功取得文件，大小为%dbytes\n", file_size);
	string content_type;
	extension = file_path.substr(dotPosition + 1);

	if (extension == "html")content_type = "text/html; charset=ISO-8859-1";
	else if (extension == "ico")content_type = "image/icon";
	else if (extension == "png")content_type = "image/png";
	else {
		cerr << "未知的文件类型：" << extension << endl;
		sendErrorHtml(500, sessionSocket);
		return 500;
	}
	file.seekg(0, std::ios::beg); // 重置文件读取位置到文件开头  
	// 准备发送响应报文
	stringstream header;  // 字符流
	// 简单的响应报文，为了减少代码量省去了很多不必要的首部行
	header << "HTTP/1.1 200 OK\r\n";
	header << "Content-Type: " << content_type << "\r\n";
	header << "Content-Length: " << file_size << "\r\n";  // 文件大小
	header << "\r\n";
	printf("sending...");
	if (sendStream(header, sessionSocket) == SERVER_RTN::FAIL)
	{
		printf("send string response error");
	}
	if (sendStream(file, sessionSocket) == SERVER_RTN::FAIL)// 将响应的文件也发送出去
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
		//清空read描述符（描述符可以视为一个集合）（清空集合）
		fd_set rfds;
		FD_ZERO(&rfds);
		//设置等待客户连接请求
		FD_SET(srvSocket, &rfds);
		for (SOCKET sessionSocket : sessionList)
		{
			//设置等待会话SOKCET可接受数据或可发送数据
			FD_SET(sessionSocket, &rfds);
		}
		//开始等待
		// select参数解释：
		// maxfdp:集合中所有文件描述符的范围，为所有文件描述符的最大值加1
		// fd_set *readfds:要监视的读文件集
		// fd_set *writefds:要进行监视的写文件集合
		// fd_set *errorfds=NULL:用于监视异常数据
		// struct timeval* timeout=NULL：设置为阻塞状态，一定要等到监视文件描述符集合中某个文件描述符发送变化才返回

		// 计算最大值
		int max_val = srvSocket;
		for (SOCKET sessionSocket : sessionList)if (max_val < sessionSocket)max_val = sessionSocket;

		int nTotal = select(max_val + 1, &rfds, NULL, NULL, NULL);

		// 返回值：
		// nTotal>0被监视的文件描述符有变化，返回的是对应位仍然为1的fd总数
		// -1:出错
		// 0;超时


		if (nTotal <= 0) {
			printf("main loop: nTotal=%d: continue", nTotal);
			continue;
		}
		//检查会话SOCKET是否有数据到来
		list<SOCKET> newList;
		for (SOCKET sessionSocket : sessionList)
		{
			//如果会话SOCKET有数据到来，则接受客户的数据
			if (FD_ISSET(sessionSocket, &rfds)) {

				memset(recvBuf, '\0', sizeof(recvBuf));
				int rtn = recv(sessionSocket, recvBuf, sizeof(recvBuf), 0);
				if (rtn > 0)
				{
					cout << "地址为";
					getClientAddrString(sessionSocket, cout);
					cout << "的sessionSocket捕获到了新的报文" << endl;
					printf("-----------------------------------------\n%s\n-----------------------------------------\n", recvBuf);


					// 此处逻辑：如果请求行=GET /index.html HTTP/1.1就把index.html返回回去
					// 无视其他的首部行
					// 从请求报文（假设其能完全存储在recvBuf中）中获取第一行
					istringstream iss(recvBuf);
					string method, url, http_version;
					iss >> method >> url >> http_version;  // 从输入中流状获取请求行的三个参数
					int code = respond(method, url, http_version, sessionSocket);
					printf("sessionSocket捕获到的报文已经响应完毕，状态码为%d\n", code);
					newList.push_back(sessionSocket);
				}
				else {
					printf("Client leaving ...\n");
					closesocket(sessionSocket);  //既然client离开了，就关闭sessionSocket
				}
			}
			else
			{
				newList.push_back(sessionSocket);
			}
		}
		sessionList.swap(newList);
		//如果srvSock收到连接请求，接受客户连接请求
		if (FD_ISSET(srvSocket, &rfds))// 如果“读”集合中srvSocket发生了变化
		{
			sockaddr_in clientAddr;//客户端地址
			clientAddr.sin_family = AF_INET;
			int addrLen = sizeof(clientAddr);
			//产生会话SOCKET
			SOCKET sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
			if (sessionSocket != INVALID_SOCKET && ioctlsocket(sessionSocket, FIONBIO, &blockMode) != SOCKET_ERROR)//把会话SOCKET设为非阻塞模式
			{
				cout << "srvSocket捕获到了新的请求，请求来源于";
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
	// 注册清理函数，使得程序退出时可以关闭所有的socket
	if (initialize() == SERVER_RTN::FAIL)return 1;
	loop();
	return 0;
}

