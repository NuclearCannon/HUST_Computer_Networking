#include "GBNReceiver.h"
#include <cstdio>
#include <cstring>



GBNReceiver::GBNReceiver(int codingLen) :expectSequenceNumberRcvd(0), mask((1 << codingLen) - 1)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.seqnum = -1;	//忽略该字段
	std::memset(lastAckPkt.payload, '.', Configuration::PAYLOAD_SIZE);
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}
GBNReceiver::~GBNReceiver()
{
	// do nothing
}


//接收报文，将被NetworkService调用
void GBNReceiver::receive(const Packet& packet)
{
	printf("GBN Receiver 收到了一个包\n");

	int checkSum = pUtils->calculateCheckSum(packet);
	//如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum != packet.checksum)
	{
		printf("GBN Receiver 拒绝了包，因为内容错误，发送上一次的ACK\n");
		pns->sendToNetworkLayer(RandomEventTarget::SENDER, lastAckPkt);
		return;
	}
	if (this->expectSequenceNumberRcvd != packet.seqnum)
	{
		printf("GBN Receiver 拒绝了包，因为这不是它想要的，发送上一次的ACK\n");
		printf("\t期待 %d 但是收到 %d\n", expectSequenceNumberRcvd, packet.seqnum);
		pns->sendToNetworkLayer(RandomEventTarget::SENDER, lastAckPkt);
		return;
	}


	pUtils->printPacket("GBN Receiver正确收到发送方的报文", packet);

	//取出Message，向上递交给应用层
	Message msg;
	memcpy(msg.data, packet.payload, Configuration::PAYLOAD_SIZE);
	pns->delivertoAppLayer(RECEIVER, msg);

	lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	pUtils->printPacket("GBN Receiver发送确认报文", lastAckPkt);
	pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
	expectSequenceNumberRcvd++;  // 接收序号+1
	expectSequenceNumberRcvd &= mask;


}

