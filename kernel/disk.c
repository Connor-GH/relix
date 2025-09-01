#include "disk.h"
#include "config.h"

#if defined(CONFIG_IDE)
#include "ide.h"
#elif defined(CONFIG_SATA)
#include "sata.h"
#else
#include "ramfs.h"
#endif

void
disk_init(void)
{
#if defined(CONFIG_IDE)
	ide_disk_init();
#elif defined(CONFIG_SATA)
	sata_disk_init();
#else
	ramfs_disk_init();
#endif
}
