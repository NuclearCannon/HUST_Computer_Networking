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


//����Ӧ�ò�������Message����NetworkServiceSimulator����,
// ������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
// �����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
bool TCPSender::send(const Message& message)
{
	// ���������������ܾ��û��ķ���
	printf("TCP Sender ���յ�����Ӧ�ò�ķ�����������Ϊ");
	print_message(message);
	printf("\n");

	if (full())
	{
		printf("TCP Sender �ܾ����ͣ���Ϊ��������\n");
		return false;
	}

	// ����ǵ�һ������Ͳ���ʼ��ʱ������ֻ���Ͳ���ʱ��
	printf("TCP Sender ���ܷ������󣬷���ǰ�����ڵ�����Ϊ\n");
	print();
	printf("\n");
	// push
	Packet& new_packet = window[(head + currentLen) % maxLen];

	std::memcpy(new_packet.payload, message.data, Configuration::PAYLOAD_SIZE);// ��Message�е���Ϣ����packet
	new_packet.acknum = -1;  // ���ͷ����͵İ�����Ҫ����ֶ�
	new_packet.seqnum = nextSeqNum;
	new_packet.checksum = pUtils->calculateCheckSum(new_packet);
	pUtils->printPacket("pushing packet", new_packet);

	currentLen++;

	// end push

	pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, new_packet);
	pUtils->printPacket("TCP Sender �����˰�:", new_packet);

	if (currentLen == 1)
	{
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, nextSeqNum);
	}

	nextSeqNum = (nextSeqNum + 1) & mask;

	printf("TCP Sender ���ͺ󣬴��ڵ�����Ϊ\n");
	print();
	printf("\n");
	return true;
}

//����ȷ��Ack������NetworkServiceSimulator����	
void TCPSender::receive(const Packet& ackPkt)
{
	// ���ȼ��������Ƿ���ȷ
	printf("TCP Sender ���յ���һ��ACK������ʱ����Ϊ\n");
	print();

	int sum = pUtils->calculateCheckSum(ackPkt);
	if (sum != ackPkt.checksum)
	{
		printf("TCP Sender ������ACK������Ϊ���ݴ���\n");
		return;// ����İ���ֱ�Ӷ�������
	}
	if (empty())
	{
		printf("TCP Sender ������ACK������Ϊ����Ϊ��\n");
		return;
	}

	// ��ȷ�İ���ȡ���е�ack,�˶�ֱ����ack�˳�ȥΪֹ
	printf("TCP Sender ������ACK����ackNum=%d\n", ackPkt.acknum);
	int ackNum = ackPkt.acknum;
	bool has;
	int headSeqNum = window[head].seqnum;
	int tailSeqNum = window[(head + currentLen - 1) % maxLen].seqnum;  // currentLen����Ϊ1����û��Ϊ�����ķ���
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
		// ������û�������
		printf("TCP Sender û���ڴ������ҵ����ACK�İ�\n");
		// ��������ACK
		if (((headSeqNum - ackNum) & mask) == 1)
		{
			counter++;
			printf("TCP Sender ��������ACK�����ǵ�%d��\n", counter);
			if (counter == 3)
			{
				printf("TCP Sender ����3������ACK����ʼ�����ش�\n");


				pUtils->printPacket("�ش�����Ϊ", window[head]);
				pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[head]);
				// �����ʱ��
				pns->stopTimer(RandomEventTarget::SENDER, headSeqNum);
				pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, headSeqNum);
				counter = 0;


			}
		}
		return;
	}

	// has=1



	printf("TCP Sender �ڳ���ǰ������%d�ļ�ʱ��\n", window[head].seqnum);
	pns->stopTimer(RandomEventTarget::SENDER, window[head].seqnum);
	printf("TCP Sender ��ʼ����\n");
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
		printf("TCP Sender ���Ƴ���ֱ����������ն�û�а�Ŀ��seqnum����");
	}
	// end pop
	printf("TCP Sender ��ɳ��ӣ����Ӻ�Ĵ���Ϊ��\n");
	print();

	// pop����֮����������ڰ�����Ҫ����һ���¼�ʱ��
	if (!empty())
	{
		printf("TCP Sender �����ʱ����%d\n", window[head].seqnum);
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, window[head].seqnum);
	}
	else
	{
		printf("TCP Sender�����Ѿ������ӿ��ˣ��ʲ������ʱ��\n");
	}

}


//Timeout handler������NetworkServiceSimulator����
void TCPSender::timeoutHandler(int seqNum)
{
	printf("TCP Sender ��ʱ�¼�(%d)\n", seqNum);
	if (empty())
	{
		printf("TCP Sender ���Ƴ�������Ϊ�գ������յ��˳�ʱ�¼�\n");
	}
	else if (seqNum == window[head].seqnum)
	{

		pUtils->printPacket("TCP Sender ���·��ͣ����������������", window[head]);
		counter = 0;
		pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[head]);
		// ������ʱ��
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, seqNum);

	}
	else
	{
		printf("TCP Sender ���Ƴ������յ���ʱ�����źţ�������seqNum��Ϊ��ǰbase\n");
	}


}
