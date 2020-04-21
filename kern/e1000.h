#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define VENDOR_ID_82540EM 0x8086
#define DEVICE_ID_82540EM 0x100E

int pci_e1000_attach(struct pci_func *pcif);

#endif  // SOL >= 6
