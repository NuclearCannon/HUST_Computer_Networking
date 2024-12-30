#include "SRSender.h"
#include <cstring>
#include <cstdio>

int SRRdtSender::getIndexBySeqNum(int seqNum) const
{
	//int ans = (head + seqNum - window[head].seqnum + maxLen) % maxLen;
	//assert(window[ans].seqnum == ans);
	//return ans;

	return getIndexBySeqNumSafe(seqNum);
}

int SRRdtSender::getIndexBySeqNumSafe(int seqNum) const
{
	for (int i = 0; i < currentLen; i++)
	{
		int j = (head + i) % maxLen;
		if (window[j].seqnum == seqNum)return j;
	}
	throw "ERROR";
	return -1;
}

bool SRRdtSender::has(int seqNum) const
{
	if (currentLen == 0)return false;

	int headSeqNum = window[head].seqnum;
	int tailSeqNum = window[(head + currentLen - 1) % maxLen].seqnum;  // currentLen至少为1，故没有为负数的风险
	if (headSeqNum <= tailSeqNum)
	{
		return headSeqNum <= seqNum && seqNum <= tailSeqNum;
	}
	else
	{
		return headSeqNum <= seqNum || seqNum <= tailSeqNum;
	}
}


void SRRdtSender::print() const
{
	printf("-------------window len=%d------------\n", currentLen);
	for (int i = 0; i != currentLen; i++)
	{
		int j = (head + i) % maxLen;
		printf("confirmed[%d]", confirmed[j]);
		pUtils->printPacket("", window[j]);
	}
	printf("------------- window end ------------\n");
}


SRRdtSender::SRRdtSender(int maxWindowLen, int codingLen) :
	head(0),
	maxLen(maxWindowLen),
	currentLen(0),
	nextSeqNum(0),
	mask((1 << codingLen) - 1),
	window(new Packet[maxWindowLen]),
	confirmed(new bool[maxWindowLen])

{

}
SRRdtSender::~SRRdtSender()
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

bool SRRdtSender::getWaitingState()
{
	return currentLen == maxLen;
}


//发送应用层下来的Message，由NetworkServiceSimulator调用,
// 如果发送方成功地将Message发送到网络层，返回true;
// 如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
bool SRRdtSender::send(const Message& message)
{
	// 如果窗口已满，则拒绝用户的发送
	printf("SR Sender 收到应用层的请求\n");
	if (getWaitingState())
	{
		printf("SR Sender 拒绝了应用层的请求，因为窗口已满\n");
		return false;
	}
	// 准备填写新的Packet
	printf("SR Sender 接受了应用层的请求，在发送前的窗口内容为：\n");
	print();

	int index = (head + currentLen) % maxLen;
	Packet& newPacket = window[index];
	confirmed[index] = false;

	std::memcpy(newPacket.payload, message.data, Configuration::PAYLOAD_SIZE);// 将Message中的信息拷入packet
	newPacket.acknum = -1;  // 发送方发送的包不需要这个字段
	newPacket.checksum = 0;// 这个值待会生成
	newPacket.seqnum = nextSeqNum;
	newPacket.checksum = pUtils->calculateCheckSum(newPacket);
	pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, newPacket);
	pUtils->printPacket("SR Sender 发送了一个新的包", newPacket);




	// 每个分组都有专用定时器
	pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, nextSeqNum);

	nextSeqNum = (nextSeqNum + 1) & mask;
	currentLen++;

	printf("SR Sender 发送后的窗口内容为：\n");
	print();
	return true;



}

//接受确认Ack，将被NetworkServiceSimulator调用	
void SRRdtSender::receive(const Packet& ackPkt)
{
	// 首先检查包内容是否正确
	printf("SR Sender 收到ACK包，当前窗口为\n");
	print();

	int sum = pUtils->calculateCheckSum(ackPkt);
	if (sum != ackPkt.checksum)
	{
		printf("SR Sender 忽略了ACK包，因为内容错误\n");
		return;// 错误的包，直接丢掉不管
	}
	// 开始检查这个数字是否在窗口中
	int ackNum = ackPkt.acknum;
	if (!has(ackNum))
	{
		// 窗口中没有这个包
		printf("SR Sender 没有在窗口中找到这个ACK(%d)的包\n", ackNum);
		return;
	}


	// 正确的包，把对应的分组设为已确定
	printf("SR Sender 接受ACK包(%d)\n", ackNum);
	confirmed[getIndexBySeqNum(ackNum)] = true;

	printf("确认后，窗口为：\n");
	print();

	if (confirmed[head])
	{
		printf("SR Sender 由于确认了队首，开始出队\n");

		while (currentLen && confirmed[head])
		{
			// 关计时器
			pns->stopTimer(RandomEventTarget::SENDER, window[head].seqnum);
			// pop window[head]
			head = (head + 1) % maxLen;
			currentLen--;
		}
		printf("出队后队列为：\n");
		print();

	}




}


//Timeout handler，将被NetworkServiceSimulator调用
void SRRdtSender::timeoutHandler(int seqNum)
{
	if (!has(seqNum))return;
	// 发送这个块
	printf("超时重传%d\n", seqNum);
	pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[getIndexBySeqNum(seqNum)]);
	// 重启计时器
	pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, seqNum);
}


