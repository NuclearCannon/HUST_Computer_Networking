#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <tchar.h>

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#pragma comment (lib,"./netsimlib.lib")


struct  Configuration {
	//�������Э��Payload���ݵĴ�С���ֽ�Ϊ��λ��
	static const int PAYLOAD_SIZE = 21;
	//��ʱ��ʱ��
	static const int TIME_OUT = 20;
};
/**
	�����Ӧ�ò����Ϣ
*/
struct  Message {
	char data[Configuration::PAYLOAD_SIZE];		//payload
	Message();
	Message(const Message& msg);
	virtual Message& operator=(const Message& msg);
	virtual ~Message();
	virtual void print();
};

/**
	���Ĳ�����㱨�Ķ�
*/
struct  Packet {
	int seqnum;										//���
	int acknum;										//ȷ�Ϻ�
	int checksum;									//У���
	char payload[Configuration::PAYLOAD_SIZE];		//payload

	Packet();
	Packet(const Packet& pkt);
	virtual Packet& operator=(const Packet& pkt);
	virtual bool operator==(const Packet& pkt) const;
	virtual ~Packet();

	virtual void print();
};

struct Tool {
	/* ��ӡPacket����Ϣ*/
	virtual void printPacket(const char* description, const Packet& packet) = 0;
	/*����һ��Packet��У���*/
	virtual int calculateCheckSum(const Packet& packet) = 0;
	/*����һ�����ȷֲ���[0-1]��������*/
	virtual double random() = 0;

	virtual ~Tool() = 0;
};

//����RdtSender�����࣬�涨�˱���ʵ�ֵ������ӿڷ���
//������������StopWaitRdtSender��GBNRdtSender������������������ľ���ʵ��
//ֻ���ǵ����䣬�����ͷ�ֻ�������ݺͽ���ȷ��
struct  RdtSender
{
	virtual bool send(const Message& message) = 0;//����Ӧ�ò�������Message����NetworkService����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ�ȷ��״̬���ʹ����������ܾ�����Message���򷵻�false
	virtual void receive(const Packet& ackPkt) = 0;//����ȷ��Ack������NetworkService����	
	virtual void timeoutHandler(int seqNum) = 0;//Timeout handler������NetworkService����
	virtual bool getWaitingState() = 0;//����RdtSender�Ƿ��ڵȴ�״̬��������ͷ����ȴ�ȷ�ϻ��߷��ʹ�������������true
	virtual ~RdtSender() = 0;
};

//����RdtReceiver�����࣬�涨�˱���ʵ�ֵ�һ���ӿڷ���
//������������StopWaitRdtReceiver��GBNRdtReceiver���������һ�������ľ���ʵ��
//ֻ���ǵ����䣬�����շ�ֻ��������
struct  RdtReceiver
{
	virtual void receive(const Packet& packet) = 0;		//���ձ��ģ�����NetworkService����	
	virtual ~RdtReceiver() = 0;
};

/* ��������¼���Ŀ��*/
enum  RandomEventTarget {
	SENDER,							//���ݷ��ͷ�
	RECEIVER						//���ݽ��շ�
};


//����NetworkService�����࣬�涨��ѧ��ʵ�ֵ�RdtSender��RdtReceiver���Ե��õĵĽӿڷ���
struct  NetworkService {

	virtual void startTimer(RandomEventTarget target, int timeOut, int seqNum) = 0;	//���ͷ�������ʱ������RdtSender����
	virtual void stopTimer(RandomEventTarget target, int seqNum) = 0;				//���ͷ�ֹͣ��ʱ������RdtSender����
	virtual void sendToNetworkLayer(RandomEventTarget target, Packet pkt) = 0;		//�����ݰ����͵�����㣬��RdtSender��RdtReceiver����
	virtual void delivertoAppLayer(RandomEventTarget target, Message msg) = 0;		//�����ݰ����ϵݽ���Ӧ�ò㣬��RdtReceiver����

	virtual void init() = 0;														//��ʼ�����绷������main�����
	virtual void start() = 0;														//�������绷������main�����
	virtual void setRtdSender(RdtSender* ps) = 0;									//���þ���ķ��ͷ�����
	virtual void setRtdReceiver(RdtReceiver* ps) = 0;								//���þ���ķ��ͷ�����
	virtual void setInputFile(const char* ifile) = 0;								//���������ļ�·��
	virtual void setOutputFile(const char* ofile) = 0;								//��������ļ�·��
	virtual ~NetworkService() = 0;
	virtual void setRunMode(int mode = 0) = 0;										//��������ģʽ��0��VERBOSEģʽ��1������ģʽ
};


extern  Tool *pUtils;						//ָ��Ψһ�Ĺ�����ʵ����ֻ��main��������ǰdelete
extern  NetworkService *pns;				//ָ��Ψһ��ģ�����绷����ʵ����ֻ��main��������ǰdelete

#endif