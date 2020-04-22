#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here
volatile uint32_t *e1000_reg;

int pci_e1000_attach(struct pci_func *pcif){
    pci_func_enable(pcif);
    e1000_reg=mmio_map_region(pcif->reg_base[0],pcif->reg_size[0]);
    assert(e1000_reg[E1000_STATUS/4]==0x80080783);
    return 1;
}