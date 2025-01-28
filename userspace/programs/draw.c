#include <gui.h>

int main(void)
{
	libgui_fill_rect(&(struct rectangle){0, 0, 640, 12}, 0xff0000);
	return 0;
}
