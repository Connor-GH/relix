#include <pci.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define MAX_PCI_DEVICES 32

int
main(void)
{
	struct pci_conf pci_conf[MAX_PCI_DEVICES] = { 0 };
	if (ioctl(0, PCIIOCGETCONF, &pci_conf) < 0) {
		perror("ioctl");
		exit(1);
	}
	for (int i = 0; i < MAX_PCI_DEVICES &&
	                (pci_conf[i].vendor_id != 0 && pci_conf[i].device_id != 0 &&
	                 pci_conf[i].subsystem_id != 0);
	     i++) {
		char **arr = libpci_device_info_alloc(pci_conf[i]);
		printf("%02x:%02x.%01x %s [%02x%02x]: %s %s [%04x:%04x] (rev %02x)",
		       pci_conf[i].bus, pci_conf[i].device, pci_conf[i].function,
		       arr != NULL ? arr[4] : "Subclass", pci_conf[i].base_class,
		       pci_conf[i].subclass, arr != NULL ? arr[0] : "Vendor",
		       arr != NULL ? arr[1] : "Device", pci_conf[i].vendor_id,
		       pci_conf[i].device_id, pci_conf[i].revision_id);

		// Hide prog-if (programming interface) if it is blank.
		if (pci_conf[i].prog_if != 0) {
			printf(" (prog-if %02x [%s])", pci_conf[i].prog_if,
			       arr != NULL ? arr[5] : "UNKNOWN");
		}
		printf("\n");

		printf("\tSubsystem: %s %s [%x:%x]\n",
		       arr != NULL ? arr[2] : "Subsystem Vendor ID",
		       arr != NULL ? arr[3] : "Subsystem ID",
		       pci_conf[i].subsystem_vendor_id, pci_conf[i].subsystem_id);
		if (arr != NULL) {
			libpci_device_info_free(arr);
		}
	}
	return 0;
}
