#pragma once
#include "kernel/include/pci.h"
#include <stdbool.h>

char **libpci_device_info_alloc(struct pci_conf pci);

void libpci_device_info_free(char **arr);
