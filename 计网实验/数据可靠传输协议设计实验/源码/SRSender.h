#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H
#include "Global.h"

class SRRdtSender :public RdtSender
{
private:
	Packet* window;
	bool* confirmed;
	int head, currentLen, nextSeqNum;
	const int maxLen, mask;
	int getIndexBySeqNum(int seqNum) const;
	int getIndexBySeqNumSafe(int seqNum) const;
	bool has(int seqNum) const;
	void print() const;
public:
	SRRdtSender(int maxWindowLen, int codingLen);
	virtual ~SRRdtSender();
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

