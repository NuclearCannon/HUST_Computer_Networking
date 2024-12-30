#ifndef TCP_SENDER_H
#define TCP_SENDER_H
#include "Global.h"

// TCP���ͷ���Ϊ��
// �����ش�����������յ�����base-1����ACK������һ��base�����Ķ�
//


class TCPSender :public RdtSender
{
private:
	Packet* window;
	int head;  // head:����ָ�룻tail:��β����һ������λ�ã�
	int currentLen;
	const int maxLen;
	int nextSeqNum;
	int counter;
	const int mask;
	bool empty() const;
	bool full() const;
	static void print_message(const Message& msg);
	void print() const;
public:
	TCPSender(int maxWindowLen, int codingLen);
	virtual ~TCPSender();
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

