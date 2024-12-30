#include "Global.h"
#include "StopWaitRdtSender.h"
#include "StopWaitRdtReceiver.h"
#include "GBNSender.h"
#include "GBNReceiver.h"
#include "SRSender.h"
#include "SRReceiver.h"
#include "TCPSender.h"
#include "TCPReceiver.h"


int main()
{
	RdtSender* ps;
	RdtReceiver* pr;
	// ������ע�͵���ͬ��Ƭ��Ȼ����룬���Եõ���ͬ�汾�ĳ���
	//ps = new GBNSender(4, 3);
	//pr = new GBNReceiver(3);

	//ps = new SRRdtSender(4, 3);
	//pr = new SRReceiver(4, 3);

	ps = new TCPSender(4, 3);
	pr = new TCPReceiver(3);


	pns->setRunMode(1);  //����ģʽ
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("./input.txt");
	pns->setOutputFile("./output.txt");
	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//ָ��Ψһ�Ĺ�����ʵ����ֻ��main��������ǰdelete
	delete pns;										//ָ��Ψһ��ģ�����绷����ʵ����ֻ��main��������ǰdelete

	return 0;
}
