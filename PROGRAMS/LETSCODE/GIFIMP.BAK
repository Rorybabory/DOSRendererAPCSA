#include <stdio.h>
#include "VGA.H"
#include "GIF.H"

int main() {
	struct image *img;
	img = load_gif("test.gif", 1);

	if (img != NULL) {
		set_graphics_mode();
		set_palette(img->palette);
		blit2vga(img->data, 0, 0, 320, 200);
		while(!kbhit());
		set_text_mode();
		return 0;
	}else {
		printf("failed to load test.gif");
		return 1;
	}
}