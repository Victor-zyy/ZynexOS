#include <kern/e1000.h>

#define TEST_PACK 0
// LAB 6: Your driver code here
#include <inc/x86.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/error.h>
#include <kern/pmap.h>

volatile uint32_t *e1000;
// The driver is responsible for 
// allocating memory for the transmit and receive queues, 
// setting up DMA descriptors, 
// and configuring the E1000 with the location of these queues, 
// but everything after that is asynchronous.
// Transmit Descriptor Array
// len of descriptor is 16 bytes 128 = 8 * 16
#define tx_desc_len 32
// Transmit Descriptor Queue
__attribute__((__aligned__(16)))
static struct tx_desc tx_desc_buffer[tx_desc_len];
// Transmit packets buffers
static char packet_buffer[tx_desc_len][1024];

#define rx_desc_len 128
__attribute__((__aligned__(16)))
static struct rx_desc rx_desc_buffer[rx_desc_len];
static char receive_buffer[rx_desc_len][1024];

static void transmit_init();
static void receive_init();

int 
pci_e1000_attach(struct pci_func *pcif)
{
	int r;
	// Step 1. after attach and then enable pci device
	pci_func_enable(pcif);

	// Step 2. MMIO communicate PCI device through memory
	cprintf("pcif->reg_base[0] : 0x%08x pcif->reg_size[0] : 0x%08x\n",
									pcif->reg_base[0], pcif->reg_size[0]);
	// BAR0 memory mapped
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("e1000_status : 0x%08x\n", e1000[E1000_STATUS]); 
	// Step 3. Transmit Initialization
	cprintf("Transmit Initialization Starting ....\n");
	transmit_init();
	// Step 4. transmit a message for test from kernel
#if TEST_PACK
	transmit_pack("zhangyanyuan", 10);
	transmit_pack("hello", 5);
#endif 
	// Step 5. Receive Initialization
	cprintf("Receive Initialization Starting ....\n");
	receive_init();
	return 1;
}

static void 
transmit_init()
{
	// Step 1. allocate a region of memory for transmit descriptor list (16-byte aligned)
	// Step 2. set TDBAL 32-bit address
	e1000[E1000_TDBAL] = (uint32_t)PADDR(tx_desc_buffer);
	e1000[E1000_TDBAH] = 0;
	cprintf("tx_desc_buffer addr : 0x%08x\n", tx_desc_buffer);
	// Step 3. set TDLEN the size of the descriptor ring in bytes ( 128-byte aligned)
	e1000[E1000_TDLEN] = sizeof(struct tx_desc) * tx_desc_len;
	// Step 4. write 0b to both the TDH/TDT to ensure the registers are initialized
	e1000[E1000_TDH] = 0x0;
	e1000[E1000_TDT] = 0x0;
	// Step 5. initialize the transmit control reg TCTL
	e1000[E1000_TCTL] |= E1000_TCTL_EN; // set enable bit 
	e1000[E1000_TCTL] |= E1000_TCTL_PSP; // set Pad Short Packet bit 
	e1000[E1000_TCTL] &= ~E1000_TCTL_CT;
	e1000[E1000_TCTL] |= (0x10 << 4);  // set threshold to 0x10h
	e1000[E1000_TCTL] &= ~E1000_TCTL_COLD;
	e1000[E1000_TCTL] |= (0x40 << 12);  // set collision distance to 0x40h
	// Step 6. Transmit IPG reg
	//e1000[E1000_TIPG] = 6 <<10 | 8 << 10 | 10;
	//we assume duplex-communicate and 802.3 IEEE
	e1000[E1000_TIPG] = 10;
	// Step 7. initialzie the tx_desc_buffer to indicate empty and ready to recycle that descriptor and use it to transmit another packet
	struct tx_desc tx;
	int i = 0;
	for(i = 0; i < tx_desc_len ; i++){
		memset(&tx, 0, sizeof(struct tx_desc));
		tx.addr = (uint32_t)PADDR(packet_buffer[i]);
		tx.upper.data |= E1000_TXD_STAT_DD;
		//cprintf("tx.addr : 0x%08x\n", tx.addr);
		tx_desc_buffer[i] = tx;
	}

}

int
transmit_pack(const char *data, int len)
{
	// Step 1. read the transmit descriptor tail
	int index = e1000[E1000_TDT];	
	// check the transmit queue being full
	struct tx_desc c_tx = tx_desc_buffer[index];	
	if((c_tx.upper.data & E1000_TXD_STAT_DD) == 0){
		// the queue is full
		// send message to user to resend the message
		cprintf("index = %d\n", index);
		return -E_NO_MEM;
	}
	// Step 2. prepare a packet that's about to send
	// struct PageInfo * pginfo = page_alloc(ALLOC_ZERO);
	// page_insert(kern_pgdir, pginfo, 

	memmove(packet_buffer[index], data, len);
	//hexdump("in kern e1000_driver:  ",packet_buffer[index], len);
	//hexdump("in kern e1000_driver_raw:  ",data, len);
	//tlb_invalidate(kern_pgdir, packet_buffer[index]);
	c_tx.lower.data |= E1000_TXD_CMD_RS;
	c_tx.lower.data |= E1000_TXD_CMD_EOP;
	//c_tx.lower.data |= E1000_TXD_CMD_IDE;
	//c_tx.lower.data |= E1000_TXD_CMD_RPS;
	c_tx.upper.fields.status = 0;
	c_tx.lower.flags.length = len;
	//cprintf("c_tx.lower : 0x%08x\n", c_tx.lower.data);
	// Step 3. copy to the descriptor buffer
	tx_desc_buffer[index] = c_tx;
	index = (index + 1) % tx_desc_len;
	// Step 4. update the TDT
	e1000[E1000_TDT] = index;

	return 0;
}

static void 
receive_init()
{
	// Step 1. Program the RAL/RAH with the desired Ethernet address (MAC)
	// by default, 
	// the card will filter out all packets. 
	// You have to configure the Receive Address Registers (RAL and RAH) 
	// with the card's own MAC address 
	// in order to accept packets addressed to that card. 
	// MAC(52:54:00:12:34:56) RAL0 RAH0
	// RAL0 -> lower 32-bit of the 48-bit MAC address
	//e1000[E1000_RAL0] = 0x52540012;
	e1000[E1000_RAL0] = 0x12005452;
	// RAH0 -> higher 16-bit of the 48-bit MAC address
	e1000[E1000_RAH0] = 0x00005634;
	//e1000[E1000_RAH0] = 0x00003456;
	e1000[E1000_RAH0] |= E1000_RAH_AV;
	// Step 2. initialize the MTA -> 0b no multicast
	e1000[E1000_MTA] = 0x0;
	// Step 3. Program the interrupt Mask set and read
	// we disable all the interrupts for receive
	// write 1b to the corresponding bit in IMC reg 
	e1000[E1000_IMS] = 0x00;
	// e1000[E1000_IMC] = 0xffffffff;
	// Step 4. allocate a region for rx_desc must be 16-byte aligned
	// then set the RDBAL/RDBAH reg
	e1000[E1000_RDBAL] = (uint32_t)PADDR(rx_desc_buffer);
	e1000[E1000_RDBAH] = 0;
	cprintf("rx_desc_buffer addr : 0x%08x\n", rx_desc_buffer);
	// Step 5. set the RDLEN reg to the size of the descriptor ring
	// must be 128-byte aligned in bytes
	e1000[E1000_RDLEN] = rx_desc_len * sizeof(struct rx_desc);
	// Step 6. initialize the RDH and RDT to 0b after power-on
	e1000[E1000_RDH] = 0x0;
	e1000[E1000_RDT] = rx_desc_len - 1;
	//Head should point to the first valid receive descriptor in the
	//descriptor ring and tail should point to one descriptor 
	//beyond the last valid descriptor in the
	//descriptor ring.
	//e1000[E1000_RDT] = 0x0;
	// Step 7. Program the Receive Control reg (RTCL)
	// long packet enable
	e1000[E1000_RCTL] &= (~E1000_RCTL_LPE);
	e1000[E1000_RCTL] |= E1000_RCTL_LBM_NO;
	// minimum threshold value
	e1000[E1000_RCTL] |= E1000_RCTL_RDMTS_HALF;
	// MO
	// BAM broadcast disable default value is 0b
	// e1000[E1000_RCTL] |= E1000_RCTL_BAM;
	// BSIZE receive buffer size
	// 1024 bytes
	e1000[E1000_RCTL] |= E1000_RCTL_SZ_1024;
	// strip CRC
	e1000[E1000_RCTL] |= E1000_RCTL_SECRC;
	// Step 8. initialzie the rx_desc_buffer 
	struct rx_desc rx;
	int i = 0;
	for(i = 0; i < rx_desc_len ; i++){
		memset(&rx, 0, sizeof(struct rx_desc));
		rx.buffer_addr = (uint32_t)PADDR(receive_buffer[i]);
		//rx.status |= E1000_RXD_STAT_DD;
		//cprintf("tx.addr : 0x%08x\n", tx.addr);
		rx_desc_buffer[i] = rx;
	}
	// receive enable
	e1000[E1000_RCTL] |= E1000_RCTL_EN;

}

// receive packet from ethernet card
int receive_pack(const char *buf, int *len)
{
	// Step 1. read the transmit descriptor tail
	int index = e1000[E1000_RDT];	
	int flag = 0;
	index = (index + 1) % rx_desc_len;
	// Step 2. check the descriptor is ready to retrive
	struct rx_desc rx = rx_desc_buffer[index];
	if((rx.status & E1000_RXD_STAT_DD) == 0){
		// not ready
		// trying to handle empty buffer
		return -E_NO_MEM;
	}
	// Step 3. copy data from card to caller
	// assert(len > rx.length);
	// cprintf("rx.addr : 0x%08x rx.length: 0x%08x\n", rx.buffer_addr, rx.length);
	memmove((void *)buf, (void*)KADDR(rx.buffer_addr), rx.length);
	// update the recv length
	*len = rx.length;
	// hexdump("recv:  ", buf, rx.length);
	// Step 4. update the RDT	
	e1000[E1000_RDT] = index;
	
	return 0;
}
void hexdump(const char *prefix, const void *data, int len)
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
