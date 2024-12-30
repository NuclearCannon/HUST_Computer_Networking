#ifndef TCP_RECEIVER_H
#define TCP_RECERIVER_H
#include "Global.h"

class TCPReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	// 期待收到的下一个报文序号
	Packet lastAckPkt;				//上次发送的确认报文
	const int mask;
public:
	TCPReceiver(int codingLen);
	virtual ~TCPReceiver();

public:
	//接收报文，将被NetworkService调用
	void receive(const Packet& packet);
};

#endif
