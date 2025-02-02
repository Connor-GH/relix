#include <pci.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <stdlib.h>

int
main(void)
{
	struct pci_conf pci_conf[MAX_PCI_DEVICES] = {0};
	if (ioctl(0, PCIIOCGETCONF, &pci_conf) < 0) {
		perror("ioctl");
		exit(1);
	}
	for (int i = 0; i < MAX_PCI_DEVICES && pci_conf[i].vendor_id != 0; i++) {
		printf("%02x:%02x:%02x %x:%x vendor %x:%x %x:%x (rev %x)\n",
				 pci_conf[i].bus,
				 pci_conf[i].device,
				 pci_conf[i].function,
         pci_conf[i].base_class,
         pci_conf[i].subclass,
				 pci_conf[i].vendor_id,
				 pci_conf[i].device_id,
				 pci_conf[i].subsystem_vendor_id,
         pci_conf[i].subsystem_id,
         pci_conf[i].revision_id);
	}
	return 0;

}
