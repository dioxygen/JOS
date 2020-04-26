#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/error.h>
#include <inc/string.h>

// LAB 6: Your driver code here
#define TX_DESC_NUM 64
#define RX_DESC_NUM 128
#define PACKET_MAX_SIZE 1518
#define RX_BUFFER_SIZE 2048
#define TDESC_STATUS_DD 1
#define RDESC_STATUS_DD 1
#define TDESC_CMD_EOP 1
#define TDESC_CMD_RS 0x8
//52:54:00:12:34:56, but this is written from lowest-order byte to highest-order byte
#define E1000_MAC_LOW 0x12005452
#define E1000_MAC_HIGH 0x5634
volatile uint32_t *e1000_reg;

struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

/* Receive Descriptor */
struct rx_desc {
    uint64_t buffer_addr; /* Address of the descriptor's data buffer */
    uint16_t length;     /* Length of data DMAed into data buffer */
    uint16_t csum;       /* Packet checksum */
    uint8_t status;      /* Descriptor status */
    uint8_t errors;      /* Descriptor Errors */
    uint16_t special;
};

__attribute__((__aligned__(sizeof(struct tx_desc))))
struct tx_desc  tx_ring[TX_DESC_NUM];

char transmit_buffer[TX_DESC_NUM][PACKET_MAX_SIZE];

__attribute__((__aligned__(sizeof(struct rx_desc))))
struct rx_desc  rx_ring[RX_DESC_NUM];

char receive_buffer[RX_DESC_NUM][RX_BUFFER_SIZE];

//按照手册声明数据结构来表示更好
static void e1000_transmit_init(){
	int i;
	for(i=0;i<TX_DESC_NUM;i++){
		tx_ring[i].addr=PADDR(transmit_buffer[i]);
		tx_ring[i].status=TDESC_STATUS_DD;
		// tx_ring[i].cso=0;
		// tx_ring[i].status&=0x0f;
	}
	e1000_reg[E1000_TDBAL/4]=PADDR(&tx_ring[0]);
	e1000_reg[E1000_TDBAH/4]=0;
	//not byte size
	e1000_reg[E1000_TDLEN/4]=TX_DESC_NUM>>3<<7;
	e1000_reg[E1000_TDH/4]=0;
	//index not byte offset
	e1000_reg[E1000_TDT/4]=0;
	e1000_reg[E1000_TCTL/4]=E1000_TCTL_EN|E1000_TCTL_PSP|(0x10<<4)|(0x40<<12);
	e1000_reg[E1000_TIPG/4]=10|(4<<10)|(6<<20);
}

int transmit_packet(void *va,uint16_t length){
	uint32_t index;
	index=e1000_reg[E1000_TDT/4];
	if(tx_ring[index].status&TDESC_STATUS_DD){
		tx_ring[index].length=length;
		tx_ring[index].cmd=TDESC_CMD_RS|TDESC_CMD_EOP;
		tx_ring[index].status&=~TDESC_STATUS_DD;
		memcpy(transmit_buffer[index],va,length);
		e1000_reg[E1000_TDT/4]=(index+1)%TX_DESC_NUM;
		return 0;
	}
	return -E_TXRING_FULL;
}

static void e1000_receive_init(){
	int i;
	for(i=0;i<RX_DESC_NUM;i++){
		rx_ring[i].buffer_addr=PADDR(receive_buffer[i]);
		rx_ring[i].status=0;
	}
	//card MAC address
	e1000_reg[E1000_RA/4]=E1000_MAC_LOW;
	e1000_reg[(E1000_RA+4)/4]=E1000_MAC_HIGH|E1000_RAH_AV;
	e1000_reg[E1000_MTA/4]=0;
	//IMS:RXT,RXO,RXDMT,RXSEQ,LSC
	e1000_reg[E1000_IMS/4]=E1000_IMS_LSC|E1000_IMS_RXSEQ|E1000_IMS_RXDMT0|E1000_IMS_RXO|E1000_IMS_RXT0;
	e1000_reg[E1000_RDBAL/4]=PADDR(&rx_ring[0]);
	e1000_reg[E1000_RDBAH/4]=0;
	e1000_reg[E1000_RDLEN/4]=RX_DESC_NUM>>3<<7;
	e1000_reg[E1000_RDH/4]=0;
	e1000_reg[E1000_RDT/4]=RX_DESC_NUM-1;
	e1000_reg[E1000_RCTL/4]=E1000_RCTL_EN|E1000_RCTL_BAM|E1000_RCTL_SZ_2048|E1000_RCTL_SECRC;
}

int receive_packet(void * va,uint16_t length){
	static uint32_t index=0;
	if(!(rx_ring[index].status&RDESC_STATUS_DD)){
		return -E_RXRING_EMPTY;
	}
	if(rx_ring[index].errors){
		cprintf("receive_packet error\n");
		return -E_RECEIVE_ERROR;
	}
	assert(rx_ring[index].length<=length);
	memcpy(va,receive_buffer[index],rx_ring[index].length);
	rx_ring[index].status=0;
	e1000_reg[E1000_RDT/4]=index;
	index=(index+1)%RX_DESC_NUM;
	return 0;
}

int pci_e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
    e1000_reg=mmio_map_region(pcif->reg_base[0],pcif->reg_size[0]);
    assert(e1000_reg[E1000_STATUS/4]==0x80080783);
	//transmit init
	e1000_transmit_init();
	e1000_receive_init();
	// char *data="I'm here!";
	// transmit_packet(data,0x2a);
	return 1;
}
