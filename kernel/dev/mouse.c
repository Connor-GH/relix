#include "dev/mouse.h"
#include "dev/ps2mouse.h"

void
dev_mouse_init(void)
{
	ps2mouseinit();
}
