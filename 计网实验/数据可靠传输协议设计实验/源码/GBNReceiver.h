#ifndef GBN_RDT_RECEIVER_H
#define GBN_RDT_RECEIVER_H
#include "Global.h"


class GBNReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	// �ڴ��յ�����һ���������
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	const int mask;
public:
	GBNReceiver(int codingLen);
	virtual ~GBNReceiver();
	//���ձ��ģ�����NetworkService����
	void receive(const Packet& packet);
};

#endif

