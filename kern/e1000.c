#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here
#define TX_DESC_NUM 64
#define PACKET_MAX_SIZE 1518

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

__attribute__((__aligned__(sizeof(struct tx_desc))))
struct tx_desc  tx_ring[TX_DESC_NUM];

char packet_buffer[TX_DESC_NUM][PACKET_MAX_SIZE];

//按照手册声明数据结构来表示更好
static void e1000_transmit_init(){
	int i;
	for(i=0;i<TX_DESC_NUM;i++){
		tx_ring[i].addr=PADDR(&packet_buffer[i][0]);
		tx_ring[i].length=PACKET_MAX_SIZE;
		tx_ring[i].cmd=1<<3;
		// tx_ring[i].cso=0;
		// tx_ring[i].status&=0x0f;
	}
	e1000_reg[E1000_TDBAL/4]=PADDR(&tx_ring[0]);
	e1000_reg[E1000_TDBAH/4]=0;
	//
	e1000_reg[E1000_TDLEN/4]=TX_DESC_NUM<<7;
	e1000_reg[E1000_TDH/4]=0;
	//index not byte offset
	e1000_reg[E1000_TDT/4]=0;
	e1000_reg[E1000_TCTL/4]=E1000_TCTL_EN|E1000_TCTL_PSP|(0x10<<4)|(0x40<<12);
	e1000_reg[E1000_TIPG/4]=10|(4<<10)|(6<<20);
}

int pci_e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
    e1000_reg=mmio_map_region(pcif->reg_base[0],pcif->reg_size[0]);
    assert(e1000_reg[E1000_STATUS/4]==0x80080783);
	//transmit init
	e1000_transmit_init();
	return 1;
}
