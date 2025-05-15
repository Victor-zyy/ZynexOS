#include "ns.h"
#include <kern/riscv/e1000.h>


extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	
	while(1){
	// Step 1. read a packet from the network server
	// how to read
	// use ipc with network server
		uint32_t whom;
		int32_t recv;
		
		recv = ipc_recv((envid_t *)&whom, (void *)REQVA, 0);

		if(whom != ns_envid){
			cprintf("NS OUTPUT: output thread got IPC message form env %x not NS\n", whom);
			continue;
		}
		// Step 2. then send the packet to the nework driver
		if(recv == NSREQ_OUTPUT){
			// the ns sent and the message is really NSREQ_OUTPUT
			struct jif_pkt *pkt = (struct jif_pkt *)REQVA;
			//cprintf("recv form %x addr : 0x%08x pkt->jp_len : %d\n", whom, pkt->jp_data, pkt->jp_len);
			//hexdump("ARP_data:\t", pkt->jp_data, pkt->jp_len);
			sys_pack_send(pkt->jp_data, pkt->jp_len);
		}

	}	
}

