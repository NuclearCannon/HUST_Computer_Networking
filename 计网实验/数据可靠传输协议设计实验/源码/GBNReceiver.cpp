#include "GBNReceiver.h"
#include <cstdio>
#include <cstring>



GBNReceiver::GBNReceiver(int codingLen) :expectSequenceNumberRcvd(0), mask((1 << codingLen) - 1)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	std::memset(lastAckPkt.payload, '.', Configuration::PAYLOAD_SIZE);
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}
GBNReceiver::~GBNReceiver()
{
	// do nothing
}


//���ձ��ģ�����NetworkService����
void GBNReceiver::receive(const Packet& packet)
{
	printf("GBN Receiver �յ���һ����\n");

	int checkSum = pUtils->calculateCheckSum(packet);
	//���У�����ȷ��ͬʱ�յ����ĵ���ŵ��ڽ��շ��ڴ��յ��ı������һ��
	if (checkSum != packet.checksum)
	{
		printf("GBN Receiver �ܾ��˰�����Ϊ���ݴ��󣬷�����һ�ε�ACK\n");
		pns->sendToNetworkLayer(RandomEventTarget::SENDER, lastAckPkt);
		return;
	}
	if (this->expectSequenceNumberRcvd != packet.seqnum)
	{
		printf("GBN Receiver �ܾ��˰�����Ϊ�ⲻ������Ҫ�ģ�������һ�ε�ACK\n");
		printf("\t�ڴ� %d �����յ� %d\n", expectSequenceNumberRcvd, packet.seqnum);
		pns->sendToNetworkLayer(RandomEventTarget::SENDER, lastAckPkt);
		return;
	}


	pUtils->printPacket("GBN Receiver��ȷ�յ����ͷ��ı���", packet);

	//ȡ��Message�����ϵݽ���Ӧ�ò�
	Message msg;
	memcpy(msg.data, packet.payload, Configuration::PAYLOAD_SIZE);
	pns->delivertoAppLayer(RECEIVER, msg);

	lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	pUtils->printPacket("GBN Receiver����ȷ�ϱ���", lastAckPkt);
	pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
	expectSequenceNumberRcvd++;  // �������+1
	expectSequenceNumberRcvd &= mask;


}

