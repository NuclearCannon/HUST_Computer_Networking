#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <tchar.h>

// TODO: 在此处引用程序需要的其他头文件
#pragma comment (lib,"./netsimlib.lib")


struct  Configuration {
	//定义各层协议Payload数据的大小（字节为单位）
	static const int PAYLOAD_SIZE = 21;
	//定时器时间
	static const int TIME_OUT = 20;
};
/**
	第五层应用层的消息
*/
struct  Message {
	char data[Configuration::PAYLOAD_SIZE];		//payload
	Message();
	Message(const Message& msg);
	virtual Message& operator=(const Message& msg);
	virtual ~Message();
	virtual void print();
};

/**
	第四层运输层报文段
*/
struct  Packet {
	int seqnum;										//序号
	int acknum;										//确认号
	int checksum;									//校验和
	char payload[Configuration::PAYLOAD_SIZE];		//payload

	Packet();
	Packet(const Packet& pkt);
	virtual Packet& operator=(const Packet& pkt);
	virtual bool operator==(const Packet& pkt) const;
	virtual ~Packet();

	virtual void print();
};

struct Tool {
	/* 打印Packet的信息*/
	virtual void printPacket(const char* description, const Packet& packet) = 0;
	/*计算一个Packet的校验和*/
	virtual int calculateCheckSum(const Packet& packet) = 0;
	/*产生一个均匀分布的[0-1]间的随机数*/
	virtual double random() = 0;

	virtual ~Tool() = 0;
};

//定义RdtSender抽象类，规定了必须实现的三个接口方法
//具体的子类比如StopWaitRdtSender、GBNRdtSender必须给出这三个方法的具体实现
//只考虑单向传输，即发送方只发送数据和接受确认
struct  RdtSender
{
	virtual bool send(const Message& message) = 0;//发送应用层下来的Message，由NetworkService调用,如果发送方成功地将Message发送到网络层，返回true;如果因为发送方处于等待确认状态或发送窗口已满而拒绝发送Message，则返回false
	virtual void receive(const Packet& ackPkt) = 0;//接受确认Ack，将被NetworkService调用	
	virtual void timeoutHandler(int seqNum) = 0;//Timeout handler，将被NetworkService调用
	virtual bool getWaitingState() = 0;//返回RdtSender是否处于等待状态，如果发送方正等待确认或者发送窗口已满，返回true
	virtual ~RdtSender() = 0;
};

//定义RdtReceiver抽象类，规定了必须实现的一个接口方法
//具体的子类比如StopWaitRdtReceiver、GBNRdtReceiver必须给出这一个方法的具体实现
//只考虑单向传输，即接收方只接收数据
struct  RdtReceiver
{
	virtual void receive(const Packet& packet) = 0;		//接收报文，将被NetworkService调用	
	virtual ~RdtReceiver() = 0;
};

/* 定义随机事件的目标*/
enum  RandomEventTarget {
	SENDER,							//数据发送方
	RECEIVER						//数据接收方
};


//定义NetworkService抽象类，规定了学生实现的RdtSender和RdtReceiver可以调用的的接口方法
struct  NetworkService {

	virtual void startTimer(RandomEventTarget target, int timeOut, int seqNum) = 0;	//发送方启动定时器，由RdtSender调用
	virtual void stopTimer(RandomEventTarget target, int seqNum) = 0;				//发送方停止定时器，由RdtSender调用
	virtual void sendToNetworkLayer(RandomEventTarget target, Packet pkt) = 0;		//将数据包发送到网络层，由RdtSender或RdtReceiver调用
	virtual void delivertoAppLayer(RandomEventTarget target, Message msg) = 0;		//将数据包向上递交到应用层，由RdtReceiver调用

	virtual void init() = 0;														//初始化网络环境，在main里调用
	virtual void start() = 0;														//启动网络环境，在main里调用
	virtual void setRtdSender(RdtSender* ps) = 0;									//设置具体的发送方对象
	virtual void setRtdReceiver(RdtReceiver* ps) = 0;								//设置具体的发送方对象
	virtual void setInputFile(const char* ifile) = 0;								//设置输入文件路径
	virtual void setOutputFile(const char* ofile) = 0;								//设置输出文件路径
	virtual ~NetworkService() = 0;
	virtual void setRunMode(int mode = 0) = 0;										//设置运行模式，0：VERBOSE模式，1：安静模式
};


extern  Tool *pUtils;						//指向唯一的工具类实例，只在main函数结束前delete
extern  NetworkService *pns;				//指向唯一的模拟网络环境类实例，只在main函数结束前delete

#endif