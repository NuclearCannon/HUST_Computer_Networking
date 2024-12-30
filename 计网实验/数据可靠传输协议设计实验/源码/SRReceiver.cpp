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


//接收报文，将被NetworkService调用
void SRReceiver::receive(const Packet& packet)
{
	//检查校验和是否正确
	printf("SR Receiver 收到一个packet，当前窗口为\n");
	print();
	if (pUtils->calculateCheckSum(packet) != packet.checksum)
	{
		printf("SR Receiver 忽视了packet，因为内容错误\n");
		return;
	}

	// 内容正确，开始检查是否需要
	int seqNum = packet.seqnum;


	int dis = (seqNum - expected) & mask;
	if (dis >= maxLen)  // 在窗口之外
	{
		int neg_dis = (expected - seqNum) & mask;
		if (neg_dis <= maxLen)
		{
			printf("SR Receiver 忽视了packet(%d)，因为目的位置不在窗口内。但因为在左侧maxLen内，还是发送了ACK\n", seqNum);
			sendAckPacket(seqNum);
		}
		else
		{
			printf("SR Receiver 忽视了packet(%d)，因为目的位置不在窗口内。因为不在左侧maxLen内，也不ACK\n", seqNum);
		}


		return;
	}



	// 否则需要
	sendAckPacket(seqNum);
	printf("SR Receiver 接受了packet(%d)，接收前窗口内容为：\n", seqNum);
	print();

	int index = (head + dis) % maxLen;
	window[index] = packet;
	confirmed[index] = true;

	printf("接收后窗口内容为：\n");
	print();

	// 发送ack包
	if (dis == 0)
	{
		// 出队
		printf("由于SR Receiver 接受了当前正期待的包，开始出队。");


		while (confirmed[head])
		{
			printf("SR Receiver 出队了(%d)\n", window[head].seqnum);
			Message msg;
			std::memcpy(msg.data, window[head].payload, Configuration::PAYLOAD_SIZE);
			pns->delivertoAppLayer(RandomEventTarget::RECEIVER, msg);

			confirmed[head] = false;
			head = (head + 1) % maxLen;
			expected = (expected + 1) & mask;
		}

		printf("出队完成，现在接收窗口为内容为：\n");
		print();
	}

}
