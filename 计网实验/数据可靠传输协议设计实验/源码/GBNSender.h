#ifndef GBN_RDT_SENDER_H
#define GBN_RDT_SENDER_H
#include "Global.h"

class GBNSender :public RdtSender
{
private:
	Packet* window;
	int head;  // head:队首指针；tail:队尾的下一个（空位置）
	int currentLen;
	const int maxLen;
	int nextSeqNum;
	const int mask;

	bool empty() const;
	bool full() const;
	static void print_message(const Message& msg);
	void print() const;
public:
	GBNSender(int maxWindowLen, int codingLen);
	virtual ~GBNSender();
	bool getWaitingState();
	//发送应用层下来的Message，由NetworkServiceSimulator调用,
	// 如果发送方成功地将Message发送到网络层，返回true;
	// 如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	bool send(const Message& message);
	//接受确认Ack，将被NetworkServiceSimulator调用	
	void receive(const Packet& ackPkt);
	//Timeout handler，将被NetworkServiceSimulator调用
	void timeoutHandler(int seqNum);
};

#endif

