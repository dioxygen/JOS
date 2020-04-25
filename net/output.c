#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	envid_t who;
	int perm;
	int res;
	while(1){
		if((res=ipc_recv(&who,&nsipcbuf,&perm))!=NSREQ_OUTPUT)
			continue;
		struct jif_pkt *pkt=(struct jif_pkt *)&nsipcbuf;
		void *va=pkt->jp_data;
		uint16_t len=pkt->jp_len;
		// cprintf("log:len=%d\n",len);
		// cprintf("log:string=%s\n",va);
		while(1){
			//如果驱动内的tx_ring满，则core network server将会被阻塞
			int r=sys_transmit_packet(va,len);
			if(r<0&&r!=-E_TXRING_FULL)
				panic("output failed with %e",r);
			else if(r==-E_TXRING_FULL)
				sys_yield();
			else
				break;
		}
	}
}
