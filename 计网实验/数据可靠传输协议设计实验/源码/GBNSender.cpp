#include "GBNSender.h"
#include <cstring>
#include <cstdio>
using namespace std;


bool GBNSender::empty() const
{
	return currentLen == 0;
}

bool GBNSender::full() const
{
	return currentLen == maxLen;
}

void GBNSender::print_message(const Message& msg)
{
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++)
	{
		if (msg.data[i] == '\n')printf("\\n");
		else if (msg.data[i] == '\r')printf("\\r");
		else if (msg.data[i] == '\0')printf("\\0");
		else putchar(msg.data[i]);
	}
}

void GBNSender::print() const
{
	printf("-------------window len=%d------------\n", currentLen);
	for (int i = 0; i != currentLen; i++)
	{
		pUtils->printPacket("window", window[(head + i) % maxLen]);
	}
	printf("------------- window end ------------\n");
}



GBNSender::GBNSender(int maxWindowLen, int codingLen) :
	head(0),
	maxLen(maxWindowLen),
	currentLen(0),
	window(new Packet[maxWindowLen]),
	nextSeqNum(0),
	mask((1 << codingLen) - 1)
{

};

GBNSender::~GBNSender()
{
	if (window)
	{
		delete[] window;
		window = nullptr;
	}
}



bool GBNSender::getWaitingState()
{
	return full();
}


//����Ӧ�ò�������Message����NetworkServiceSimulator����,
// ������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
// �����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
bool GBNSender::send(const Message& message)
{
	// ���������������ܾ��û��ķ���
	printf("GBN Sender ���յ�����Ӧ�ò�ķ�����������Ϊ");
	print_message(message);
	printf("\n");

	if (full())
	{
		printf("GBN Sender �ܾ����ͣ���Ϊ��������\n");
		return false;
	}

	// ����ǵ�һ������Ͳ���ʼ��ʱ������ֻ���Ͳ���ʱ��
	printf("GBN Sender ���ܷ������󣬷���ǰ�����ڵ�����Ϊ\n");
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
	pUtils->printPacket("GBN Sender �����˰�:", new_packet);

	if (currentLen == 1)
	{
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, nextSeqNum);
	}

	nextSeqNum = (nextSeqNum + 1) & mask;

	printf("GBN Sender ���ͺ󣬴��ڵ�����Ϊ\n");
	print();
	printf("\n");
	return true;
}

//����ȷ��Ack������NetworkServiceSimulator����	
void GBNSender::receive(const Packet& ackPkt)
{
	// ���ȼ��������Ƿ���ȷ
	printf("GBN Sender ���յ���һ��ACK������ǰ����Ϊ\n");
	print();

	int sum = pUtils->calculateCheckSum(ackPkt);
	if (sum != ackPkt.checksum)
	{
		printf("GBN Sender ������ACK������Ϊ���ݴ���\n");
		return;// ����İ���ֱ�Ӷ�������
	}
	if (empty())
	{
		printf("GBN Sender ������ACK������Ϊ����Ϊ��\n");
		return;
	}
	// ��ȷ�İ���ȡ���е�ack,�˶�ֱ����ack�˳�ȥΪֹ
	printf("GBN Sender ������ACK����ackNum=%d\n", ackPkt.acknum);
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
		// do nothing
		printf("GBN Sender û���ڴ������ҵ����ACK�İ�\n");
		return;
	}


	printf("GBN Sender �ڳ���ǰ������%d�ļ�ʱ��\n", window[head].seqnum);
	pns->stopTimer(RandomEventTarget::SENDER, window[head].seqnum);

	printf("GBN Sender ��ʼ����\n");

	// pop
	int last_pop = -1;
	while (currentLen && last_pop != ackNum)
	{
		last_pop = window[head].seqnum;
		head = (head + 1) % maxLen;
		currentLen--;
	}
	if (currentLen == 0 && last_pop != ackNum)
	{
		printf("GBN Sender ���Ƴ���ֱ����������ն�û�а�Ŀ��seqnum����");
	}
	// end pop
	printf("GBN Sender ��ɳ��ӣ����Ӻ�Ĵ���Ϊ��\n");
	print();

	// pop����֮����������ڰ�����Ҫ����һ���¼�ʱ��
	if (!empty())
	{
		printf("GBN Sender �����ʱ����%d\n", window[head].seqnum);
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, window[head].seqnum);
	}
	else
	{
		printf("GBN Sender�����Ѿ������ӿ��ˣ��ʲ������ʱ��\n");
	}





}


//Timeout handler������NetworkServiceSimulator����
void GBNSender::timeoutHandler(int seqNum)
{
	printf("GBN Sender ��ʱ�¼�(%d)\n", seqNum);
	if (empty())
	{
		printf("GBN Sender ���Ƴ�������Ϊ�գ������յ��˳�ʱ�¼�\n");
	}
	else if (seqNum == window[head].seqnum)
	{
		printf("GBN Sender ׼����Ӧ��ʱ\n");


		for (int i = 0; i != currentLen; i++)
		{
			int j = (head + i) % maxLen;
			pUtils->printPacket("GBN Sender ���·���", window[j]);
			pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[j]);
		}
		// ������ʱ��
		pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, seqNum);
	}
	else
	{
		printf("GBN Sender ���Ƴ������յ���ʱ�����źţ�������seqNum��Ϊ��ǰbase\n");
	}


}





