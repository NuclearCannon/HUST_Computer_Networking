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
	int tailSeqNum = window[(head + currentLen - 1) % maxLen].seqnum;  // currentLen����Ϊ1����û��Ϊ�����ķ���
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


//����Ӧ�ò�������Message����NetworkServiceSimulator����,
// ������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
// �����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
bool SRRdtSender::send(const Message& message)
{
	// ���������������ܾ��û��ķ���
	printf("SR Sender �յ�Ӧ�ò������\n");
	if (getWaitingState())
	{
		printf("SR Sender �ܾ���Ӧ�ò��������Ϊ��������\n");
		return false;
	}
	// ׼����д�µ�Packet
	printf("SR Sender ������Ӧ�ò�������ڷ���ǰ�Ĵ�������Ϊ��\n");
	print();

	int index = (head + currentLen) % maxLen;
	Packet& newPacket = window[index];
	confirmed[index] = false;

	std::memcpy(newPacket.payload, message.data, Configuration::PAYLOAD_SIZE);// ��Message�е���Ϣ����packet
	newPacket.acknum = -1;  // ���ͷ����͵İ�����Ҫ����ֶ�
	newPacket.checksum = 0;// ���ֵ��������
	newPacket.seqnum = nextSeqNum;
	newPacket.checksum = pUtils->calculateCheckSum(newPacket);
	pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, newPacket);
	pUtils->printPacket("SR Sender ������һ���µİ�", newPacket);




	// ÿ�����鶼��ר�ö�ʱ��
	pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, nextSeqNum);

	nextSeqNum = (nextSeqNum + 1) & mask;
	currentLen++;

	printf("SR Sender ���ͺ�Ĵ�������Ϊ��\n");
	print();
	return true;



}

//����ȷ��Ack������NetworkServiceSimulator����	
void SRRdtSender::receive(const Packet& ackPkt)
{
	// ���ȼ��������Ƿ���ȷ
	printf("SR Sender �յ�ACK������ǰ����Ϊ\n");
	print();

	int sum = pUtils->calculateCheckSum(ackPkt);
	if (sum != ackPkt.checksum)
	{
		printf("SR Sender ������ACK������Ϊ���ݴ���\n");
		return;// ����İ���ֱ�Ӷ�������
	}
	// ��ʼ�����������Ƿ��ڴ�����
	int ackNum = ackPkt.acknum;
	if (!has(ackNum))
	{
		// ������û�������
		printf("SR Sender û���ڴ������ҵ����ACK(%d)�İ�\n", ackNum);
		return;
	}


	// ��ȷ�İ����Ѷ�Ӧ�ķ�����Ϊ��ȷ��
	printf("SR Sender ����ACK��(%d)\n", ackNum);
	confirmed[getIndexBySeqNum(ackNum)] = true;

	printf("ȷ�Ϻ󣬴���Ϊ��\n");
	print();

	if (confirmed[head])
	{
		printf("SR Sender ����ȷ���˶��ף���ʼ����\n");

		while (currentLen && confirmed[head])
		{
			// �ؼ�ʱ��
			pns->stopTimer(RandomEventTarget::SENDER, window[head].seqnum);
			// pop window[head]
			head = (head + 1) % maxLen;
			currentLen--;
		}
		printf("���Ӻ����Ϊ��\n");
		print();

	}




}


//Timeout handler������NetworkServiceSimulator����
void SRRdtSender::timeoutHandler(int seqNum)
{
	if (!has(seqNum))return;
	// ���������
	printf("��ʱ�ش�%d\n", seqNum);
	pns->sendToNetworkLayer(RandomEventTarget::RECEIVER, window[getIndexBySeqNum(seqNum)]);
	// ������ʱ��
	pns->startTimer(RandomEventTarget::SENDER, Configuration::TIME_OUT, seqNum);
}


