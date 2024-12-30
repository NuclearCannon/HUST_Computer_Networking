#ifndef TCP_RECEIVER_H
#define TCP_RECERIVER_H
#include "Global.h"

class TCPReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	// �ڴ��յ�����һ���������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	const int mask;
public:
	TCPReceiver(int codingLen);
	virtual ~TCPReceiver();

public:
	//���ձ��ģ�����NetworkService����
	void receive(const Packet& packet);
};

#endif
