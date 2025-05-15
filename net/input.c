#include "ns.h"

extern union Nsipc nsipcbuf;
static void hexdump(const char *prefix, const void *data, int len);

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
	sys_page_alloc(0, (void *)REQVA, PTE_U | PTE_V | PTE_W | PTE_R);
	struct jif_pkt * pkt = (struct jif_pkt *)REQVA;
	int ok = 0;
	while(1){
		// Step 1. receive a packet from device driver ethernet card
		if(ok)
			sys_page_alloc(0, (void *)REQVA, PTE_U | PTE_V | PTE_W | PTE_R);
		int r = sys_pack_recv(pkt->jp_data, &(pkt->jp_len));
		if(r == -E_WAIT){
			ok = 0;
			continue;
		}
		// cprintf(" package recv get into here jp->len : %d\n", nsipcbuf.pkt.jp_len);
		// receive packet
		// Step 2. send the packet to core server
		// hexdump("input_env: ", nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
		ipc_send(ns_envid, NSREQ_INPUT, (void *)REQVA, PTE_V | PTE_U | PTE_W | PTE_R);
		// cprintf(" send recv get into here jp->len : %d\n", nsipcbuf.pkt.jp_len);
		sys_page_unmap(0, (void *)REQVA);
		ok = 1;
		sys_yield();
	}
}
				
				
static void
hexdump(const char *prefix, const void *data, int len)
{
	int i;
	char buf[80];
	char *end = buf + sizeof(buf);
	char *out = NULL;
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			out = buf + snprintf(buf, end - buf,
					     "%s%04x   ", prefix, i);
		out += snprintf(out, end - out, "%02x", ((uint8_t*)data)[i]);
		if (i % 16 == 15 || i == len - 1)
			cprintf("%.*s\n", out - buf, buf);
		if (i % 2 == 1)
			*(out++) = ' ';
		if (i % 16 == 7)
			*(out++) = ' ';
	}
}


