#ifndef GBN_RDT_SENDER_H
#define GBN_RDT_SENDER_H
#include "Global.h"

class GBNSender :public RdtSender
{
private:
	Packet* window;
	int head;  // head:����ָ�룻tail:��β����һ������λ�ã�
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

