#include "dev/sd.h"

#include "config.h"

#if defined(CONFIG_IDE)
#include "ide.h"
#elif defined(CONFIG_SATA)
// not implemented
#include "sata.h"
#else
// not implemented
#include "ramfs.h"
#endif

void
dev_sd_init(void)
{
#if defined(CONFIG_IDE)
	ide_init();
#elif defined(CONFIG_SATA)
	sata_init();
#else
	ramfs_init();
#endif
}
