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
	//����Ӧ�ò�������Message����NetworkServiceSimulator����,
	// ������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
	// �����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	bool send(const Message& message);
	//����ȷ��Ack������NetworkServiceSimulator����	
	void receive(const Packet& ackPkt);
	//Timeout handler������NetworkServiceSimulator����
	void timeoutHandler(int seqNum);
};

#endif

