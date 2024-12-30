#include "SRReceiver.h"
#include <cstring>
#include <cstdio>



int SRReceiver::distance(int seqNum) const
{
	return (seqNum - expected) & mask;
}

void SRReceiver::sendAckPacket(int seqNum)
{
	ackPkt.acknum = seqNum;
	ackPkt.checksum = pUtils->calculateCheckSum(ackPkt);
	pns->sendToNetworkLayer(RandomEventTarget::SENDER, ackPkt);

}

void SRReceiver::print() const
{
	printf("-------------rcv window------------\n");
	for (int i = 0; i != maxLen; i++)
	{
		int j = (head + i) % maxLen;
		printf("%d:", (expected + i) & mask);
		if (confirmed[j])
		{
			pUtils->printPacket("received", window[j]);
		}
		else
		{
			printf("havn't receive\n");

		}

	}
	printf("-------------rcv window end ------------\n");
}


SRReceiver::SRReceiver(int maxWindowLen, int codingLen) :
	head(0),
	maxLen(maxWindowLen),
	expected(0),
	mask((1 << codingLen) - 1),
	window(new Packet[maxWindowLen]),
	confirmed(new bool[maxWindowLen])
{

	ackPkt.seqnum = -1;
	memset(ackPkt.payload, '.', Configuration::PAYLOAD_SIZE);
	for (int i = 0; i < maxWindowLen; i++)confirmed[i] = false;
}
SRReceiver::~SRReceiver()
{
	if (window)
	{
		delete[] window;
		window = nullptr;
	}
	if (confirmed)
	{
		delete[] confirmed;
		confirmed = nullptr;
	}
}


//���ձ��ģ�����NetworkService����
void SRReceiver::receive(const Packet& packet)
{
	//���У����Ƿ���ȷ
	printf("SR Receiver �յ�һ��packet����ǰ����Ϊ\n");
	print();
	if (pUtils->calculateCheckSum(packet) != packet.checksum)
	{
		printf("SR Receiver ������packet����Ϊ���ݴ���\n");
		return;
	}

	// ������ȷ����ʼ����Ƿ���Ҫ
	int seqNum = packet.seqnum;


	int dis = (seqNum - expected) & mask;
	if (dis >= maxLen)  // �ڴ���֮��
	{
		int neg_dis = (expected - seqNum) & mask;
		if (neg_dis <= maxLen)
		{
			printf("SR Receiver ������packet(%d)����ΪĿ��λ�ò��ڴ����ڡ�����Ϊ�����maxLen�ڣ����Ƿ�����ACK\n", seqNum);
			sendAckPacket(seqNum);
		}
		else
		{
			printf("SR Receiver ������packet(%d)����ΪĿ��λ�ò��ڴ����ڡ���Ϊ�������maxLen�ڣ�Ҳ��ACK\n", seqNum);
		}


		return;
	}



	// ������Ҫ
	sendAckPacket(seqNum);
	printf("SR Receiver ������packet(%d)������ǰ��������Ϊ��\n", seqNum);
	print();

	int index = (head + dis) % maxLen;
	window[index] = packet;
	confirmed[index] = true;

	printf("���պ󴰿�����Ϊ��\n");
	print();

	// ����ack��
	if (dis == 0)
	{
		// ����
		printf("����SR Receiver �����˵�ǰ���ڴ��İ�����ʼ���ӡ�");


		while (confirmed[head])
		{
			printf("SR Receiver ������(%d)\n", window[head].seqnum);
			Message msg;
			std::memcpy(msg.data, window[head].payload, Configuration::PAYLOAD_SIZE);
			pns->delivertoAppLayer(RandomEventTarget::RECEIVER, msg);

			confirmed[head] = false;
			head = (head + 1) % maxLen;
			expected = (expected + 1) & mask;
		}

		printf("������ɣ����ڽ��մ���Ϊ����Ϊ��\n");
		print();
	}

}
