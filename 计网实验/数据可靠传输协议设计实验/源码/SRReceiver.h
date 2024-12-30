
#ifndef SR_RDT_RECEIVER_H
#define SR_RDT_RECEIVER_H
#include "Global.h"


class SRReceiver :public RdtReceiver
{
private:
	Packet* window;
	Packet ackPkt;
	bool* confirmed;
	int head, expected;
	const int maxLen, mask;
	int distance(int seqNum) const;
	void sendAckPacket(int seqNum);
	void print() const;
public:
	SRReceiver(int maxWindowLen, int codingLen);
	virtual ~SRReceiver();
	//接收报文，将被NetworkService调用
	void receive(const Packet& packet);
};

#endif

