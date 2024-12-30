#include "TCPSender.h"
#include <cstring>
#include <cstdio>
bool TCPSender::empty() const
{
	return currentLen == 0;
}

bool TCPSender::full() const
{
	return currentLen == maxLen;
}

void TCPSender::print_message(const Message& msg)
{
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++)
	{
		if (msg.data[i] == '\n')printf("\\n");
		else if (msg.data[i] == '\r')printf("\\r");
		else if (msg.data[i] == '\0')printf("\\0");
		else putchar(msg.data[i]);
	}
}

void TCPSender::print() const
{
	printf("-------------window len=%d------------\n", currentLen);
	for (int i = 0; i != currentLen; i++)
	{
		pUtils->printPacket("window", window[(head + i) % maxLen]);
	}
	printf("------------- window end ------------\n");
}



TCPSender::TCPSender(int maxWindowLen, int codingLen) :
	head(0),
	maxLen(maxWindowLen),
	currentLen(0),
	window(new Packet[maxWindowLen]),
	nextSeqNum(0),
	mask((1 << codingLen) - 1),
	counter(0)
{

};

TCPSender::~TCPSender()
{
	if (window)
	{
		delete[] window;
		window = nullptr;
	}
}



bool TCPSender::getWaitingState()
{
	return full();
}


//发送应用层下来的Message，由NetworkServiceSimulator调用,
// 如果发送方成功地将Message发送到网络层，返回true;
// 如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
bool TCPSender::send(const Message& message)
{
	// 如果窗口已满，则拒绝用户的发送
	printf("TCP Sender 接收到来自应用层的发送请求，内容为");
	print_message(message);
	printf("\n");

	if (full())
	{
		printf("TCP Sender 拒绝发送，因为窗口已满\n");
		return false;
	}

	// 如果是第一个项，则发送并开始计时。否则，只发送不计时。
	printf("TCP Sender 接受发送请求，发送前，窗口的内容为\n");
	print();
	printf("\n");
	// push
	Packet& new_packet = window[(head + currentLen) % maxLen];

	std::memcpy(new_packet.payload, message.data, Configuration::PAYLOAD_SIZE);// 将Message中的信息拷入packet
	new_packet.acknum = -1;  // 发送方发送的包不需要这个字段
	new_packet.seqnum = nextSeqNum;
	new_packet.checksum = pUtils->calculateCheckSum(new_packet);
	pUtils->printPacket("pushing packet", new_packet);

	currentLen++;

	// end push

	pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, new_packet);
	pUtils->printPacket("TCP Sender 发送了包:", new_packet);

	if (currentLen == 1)
	{
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, nextSeqNum);
	}

	nextSeqNum = (nextSeqNum + 1) & mask;

	printf("TCP Sender 发送后，窗口的内容为\n");
	print();
	printf("\n");
	return true;
}

//接受确认Ack，将被NetworkServiceSimulator调用	
void TCPSender::receive(const Packet& ackPkt)
{
	// 首先检查包内容是否正确
	printf("TCP Sender 接收到了一个ACK包，此时窗口为\n");
	print();

	int sum = pUtils->calculateCheckSum(ackPkt);
	if (sum != ackPkt.checksum)
	{
		printf("TCP Sender 忽略了ACK包，因为内容错误\n");
		return;// 错误的包，直接丢掉不管
	}
	if (empty())
	{
		printf("TCP Sender 忽略了ACK包，因为窗口为空\n");
		return;
	}

	// 正确的包，取其中的ack,退队直到把ack退出去为止
	printf("TCP Sender 接受了ACK包，ackNum=%d\n", ackPkt.acknum);
	int ackNum = ackPkt.acknum;
	bool has;
	int headSeqNum = window[head].seqnum;
	int tailSeqNum = window[(head + currentLen - 1) % maxLen].seqnum;  // currentLen至少为1，故没有为负数的风险
	if (headSeqNum <= tailSeqNum)
	{
		has = headSeqNum <= ackNum && ackNum <= tailSeqNum;
	}
	else
	{
		has = headSeqNum <= ackNum || ackNum <= tailSeqNum;
	}

	if (!has)
	{
		// 窗口中没有这个包
		printf("TCP Sender 没有在窗口中找到这个ACK的包\n");
		// 考虑冗余ACK
		if (((headSeqNum - ackNum) & mask) == 1)
		{
			counter++;
			printf("TCP Sender 发现冗余ACK，这是第%d个\n", counter);
			if (counter == 3)
			{
				printf("TCP Sender 集齐3个冗余ACK，开始快速重传\n");


				pUtils->printPacket("重传对象为", window[head]);
				pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[head]);
				// 重设计时器
				pns->stopTimer(RandomEventTarget::SENDER, headSeqNum);
				pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, headSeqNum);
				counter = 0;


			}
		}
		return;
	}

	// has=1



	printf("TCP Sender 在出队前结束了%d的计时器\n", window[head].seqnum);
	pns->stopTimer(RandomEventTarget::SENDER, window[head].seqnum);
	printf("TCP Sender 开始出队\n");
	// pop
	int last_pop = -1;
	while (currentLen && last_pop != ackNum)
	{
		last_pop = window[head].seqnum;
		head = (head + 1) % maxLen;
		currentLen--;
		counter = 0;
	}
	if (currentLen == 0 && last_pop != ackNum)
	{
		printf("TCP Sender 疑似出错：直到将队列清空都没有把目标seqnum出队");
	}
	// end pop
	printf("TCP Sender 完成出队，出队后的窗口为：\n");
	print();

	// pop完了之后，如果还存在包，需要设置一个新计时器
	if (!empty())
	{
		printf("TCP Sender 重设计时器至%d\n", window[head].seqnum);
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, window[head].seqnum);
	}
	else
	{
		printf("TCP Sender窗口已经被出队空了，故不重设计时器\n");
	}

}


//Timeout handler，将被NetworkServiceSimulator调用
void TCPSender::timeoutHandler(int seqNum)
{
	printf("TCP Sender 超时事件(%d)\n", seqNum);
	if (empty())
	{
		printf("TCP Sender 疑似出错：窗口为空，但是收到了超时事件\n");
	}
	else if (seqNum == window[head].seqnum)
	{

		pUtils->printPacket("TCP Sender 重新发送，并清零冗余计数器", window[head]);
		counter = 0;
		pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[head]);
		// 重启计时器
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, seqNum);

	}
	else
	{
		printf("TCP Sender 疑似出错：接收到计时结束信号，但是其seqNum不为当前base\n");
	}


}
