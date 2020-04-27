#include "ns.h"

extern union Nsipc nsipcbuf;

void
sleep(int msec)
{
	unsigned now = sys_time_msec();
	unsigned end = now + msec;

	if ((int)now < 0 && (int)now > -MAXERROR)
		panic("sys_time_msec: %e", (int)now);
	if (end < now)
		panic("sleep: wrap");

	while (sys_time_msec() < end)
		sys_yield();
}

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	struct jif_pkt *pkt=(struct jif_pkt *)&nsipcbuf;
	char buf[2048];
	uint32_t size;
	while(1){
		int res=sys_receive_packet(buf,&size);
		if(res<0)
			continue;
		else{
			memcpy(pkt->jp_data,buf,size);
			pkt->jp_len=size;
			ipc_send(ns_envid,NSREQ_INPUT,pkt,PTE_U|PTE_P);
			//wait for a while
			sleep(30);
		}
	}
}
