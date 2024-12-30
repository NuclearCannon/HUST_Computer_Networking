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
	// 在这里注释掉不同的片段然后编译，可以得到不同版本的程序
	//ps = new GBNSender(4, 3);
	//pr = new GBNReceiver(3);

	//ps = new SRRdtSender(4, 3);
	//pr = new SRReceiver(4, 3);

	ps = new TCPSender(4, 3);
	pr = new TCPReceiver(3);


	pns->setRunMode(1);  //安静模式
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("./input.txt");
	pns->setOutputFile("./output.txt");
	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//指向唯一的工具类实例，只在main函数结束前delete
	delete pns;										//指向唯一的模拟网络环境类实例，只在main函数结束前delete

	return 0;
}
