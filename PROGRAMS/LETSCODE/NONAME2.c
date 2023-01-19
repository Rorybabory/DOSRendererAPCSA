#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "math2.h"
#include "chars.h"

#define NUM_COLORS 256
#define SET_MODE 0x0
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03
#define SCREEN_HEIGHT 200
#define SCREEN_WIDTH 320
#define PALETTE_INDEX 0x3c8
#define PALETTE_DATA 0x3c9
#define INPUT_STATUS 0x03DA
#define VRETRACE_BIT 0x08
#define DISPLAY_ENABLE 0x01


/* Mouse */
#define MOUSE_INT 0x33
#define INIT_MOUSE 0x00
#define SHOW_MOUSE 0x01
#define HIDE_MOUSE 0x02
#define GET_MOUSE_STATUS 0x03

#define BLACK 0x00;
#define WHITE 0x0F;


#define SC_INDEX      0x03c4
#define SC_DATA       0x03c5

/* VGA CRT controller */
#define CRTC_INDEX    0x03d4
#define CRTC_DATA     0x03d5

#define MEMORY_MODE   0x04
#define UNDERLINE_LOC 0x14
#define MODE_CONTROL  0x17

#define MAP_MASK 0x02


#define SCREEN_SIZE         (word)(SCREEN_WIDTH*SCREEN_HEIGHT)


byte far *VGA = (byte far *)(0xA0000000L);

word visible_page=0;
word non_visible_page=SCREEN_SIZE/4;

byte current_sector = 0;

byte active_page = 0;
byte newpage = 0;
#define SETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)=c
#define GETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))


void wait_for_retrace() {
	while ((inp(INPUT_STATUS) & VRETRACE_BIT));
	while (!(inp(INPUT_STATUS) & VRETRACE_BIT));
}
void select_plane(byte plane) {
	outp(SC_INDEX, MAP_MASK);
	outp(0x3c4, 0x02); /* go to the map mask register*/
	outp(0x3c5, 1 << plane);
}
void set_pix_u(int x, int y, byte c) {
	newpage = 1 << (x&3);
	if (active_page != newpage) {
		active_page = newpage;
	}
	
	outp(SC_INDEX, MAP_MASK);
	outp(SC_DATA,  active_page );

	VGA[non_visible_page + (y<<6)+(y<<4)+(x>>2)]=c;
}
void page_flip() {
	word high_address,low_address;
	word temp;

	/* swap pointers for the pages */
	temp = visible_page;
	visible_page = non_visible_page;
	non_visible_page = temp;
	
	high_address = 0x0C | (visible_page & 0xff00);
	low_address = 0x0D  | (visible_page << 8);
	
	while ((inp(INPUT_STATUS) & DISPLAY_ENABLE));
	outpw(CRTC_INDEX, high_address);
	outpw(CRTC_INDEX, low_address);
	while (!(inp(INPUT_STATUS) & VRETRACE_BIT));

}
void set_mode(byte mode) {
	union REGS regs;
	regs.h.ah = SET_MODE;
	regs.h.al = mode;
	int86(VIDEO_INT, &regs, &regs);
}
void set_mode_x() {
	word i;

	union REGS regs;
	regs.h.ah = SET_MODE;
	regs.h.al = VGA_256_COLOR_MODE;
	int86(VIDEO_INT, &regs, &regs);
	
	outp(SC_INDEX, MEMORY_MODE);
	outp(SC_DATA, 0x06);
	
	
	outp(CRTC_INDEX, UNDERLINE_LOC);
	outp(CRTC_DATA, 0x00);
	
	outp(CRTC_INDEX, MODE_CONTROL);
	outp(CRTC_DATA, 0xe3);
}

int init_mouse() {
	union REGS regs;
	regs.x.ax = INIT_MOUSE;
	int86(MOUSE_INT, &regs, &regs);
	return 0xFFFF == regs.x.ax;
}
void show_mouse() {
	union REGS regs;
	regs.x.ax = SHOW_MOUSE;
	int86(MOUSE_INT, &regs, &regs);
}
void hide_mouse() {
	union REGS regs;
	regs.x.ax = HIDE_MOUSE;
	int86(MOUSE_INT, &regs, &regs);
}
void get_mouse(int * x, int * y, int * left, int * right) {
	union REGS regs;
	regs.x.ax = GET_MOUSE_STATUS;
	int86(MOUSE_INT, &regs, &regs);
	
	*x = regs.x.cx / 2;
	*y = regs.x.dx;
	*left = regs.x.bx & 0x1;
	*right = regs.x.bx & 0x2;
}

void draw_line(line ln) {
	int dx = abs(ln.v1.x - ln.v0.x);
	int sx = ln.v0.x < ln.v1.x ? 1 : -1;
	
	int dy = -abs(ln.v1.y - ln.v0.y);
	int sy = ln.v0.y < ln.v1.y ? 1 : -1;
	
	int error = dx + dy;
	
	int x0 = ln.v0.x;
	int x1 = ln.v1.x;
	int y0 = ln.v0.y;
	int y1 = ln.v1.y;
	
	int e2 = 0;
	
	while (1) {
		if (x0 < 320 && x0 > 0 && y0 < 200 && y0 > 0) {
			set_pix_u(x0, y0, ln.c);
		}
		
		if (x0 == x1 && y0 == y1) {break;}
		e2 = 2 * error;
		if (e2 >= dy) {
			if (x0 == x1) break;
			error = error + dy;
			x0 = x0 + sx;
		}
		if (e2 <= dx) {
			if (y0 == y1) {break;}
			error = error + dx;
			y0 = y0 + sy;
		}
	}
}
void draw_triangle_wire(triangle tri) {
  line ln;
  ln.v0 = vec3tovec2(tri.v[0]);
  ln.v1 = vec3tovec2(tri.v[1]);
  ln.c = 0x47;
  draw_line(ln);
  ln.v0 = vec3tovec2(tri.v[1]);
  ln.v1 = vec3tovec2(tri.v[2]);
  ln.c = 0x47;
  draw_line(ln);
  ln.v0 = vec3tovec2(tri.v[2]);
  ln.v1 = vec3tovec2(tri.v[0]);
  ln.c = 0x47;
  draw_line(ln);

}
int orient2d(ivec3 a, ivec3 b, ivec3 c)
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}
void draw_triangle(triangle tri) {
	vec3 v0 = tri.v[0];
	vec3 v1 = tri.v[1];
	vec3 v2 = tri.v[2];
	float e1;
	float e2;
	float e3;

	/*precached edge values*/

	float e11;
	float e21;
	float e31;

	float e12;
	float e22;
	float e32;

	


	int x,y;
	vec2 max = {0, 0};
	vec2 min = {320, 240};

	float x01;
	float x12;
	float x02;

	float area;
	
	float grayscale = 0;
		
	if (v0.x > max.x) {max.x = v0.x; }
	if (v1.x > max.x) {max.x = v1.x; }
	if (v2.x > max.x) {max.x = v2.x; }
	
	if (v0.x < min.x) {min.x = v0.x; }
	if (v1.x < min.x) {min.x = v1.x; }
	if (v2.x < min.x) {min.x = v2.x; }
	
	if (v0.y > max.y) {max.y = v0.y; }
	if (v1.y > max.y) {max.y = v1.y; }
	if (v2.y > max.y) {max.y = v2.y; }
	
	if (v0.y < min.y) {min.y = v0.y; }
	if (v1.y < min.y) {min.y = v1.y; }
	if (v2.y < min.y) {min.y = v2.y; }
	
	area = (v2.x - v0.x) * (v1.y - v0.y) - (v2.y - v0.y) * (v1.x - v0.x);
	e11 = (v0.x - v1.x);
	e21 = (v1.x - v2.x);
	e31 = (v2.x - v0.x);

	e12 = (v0.y - v1.y);
	e22 = (v1.y - v2.y);
	e32 = (v2.y - v0.y);
	for (x = min.x; x < max.x; x++) {

		outp(SC_INDEX, MAP_MASK);
		outp(SC_DATA,  1 << (x&3) );

		for (y = min.y; y < max.y; y++) {
			e1 = e11 * (y - v0.y) - e12 * (x - v0.x);
			e2 = e21 * (y - v1.y) - e22 * (x - v1.x);
			e3 = e31 * (y - v2.y) - e32 * (x - v2.x);

			if (e1 >= 0 &&
				 e2 >= 0 &&
				 e3 >= 0) {
				e1 /= area;
				e2 /= area;
				e3 /= area;

				e2 *= 16.0f;


				VGA[non_visible_page + (y<<6)+(y<<4)+(x>>2)]=((int)e2)+16;
			}
			
			
			
		}
	}
/*
	set_pix_u(v0.x, v0.y, 0x20);
	set_pix_u(v1.x, v1.y, 0x30);
	set_pix_u(v2.x, v2.y, 0x40);
*/	
	
	
}
void draw_bottomflat_triangle(ivec3 v0, ivec3 v1, ivec3 v2, int middleisleft) {
	float invslope1 = ((float)v1.x - (float)v0.x) / ((float)v1.y - (float)v0.y);
	float invslope2 = ((float)v2.x - (float)v0.x) / ((float)v2.y - (float)v0.y);
	line ln;
	float curx1 = v0.x;
	float curx2 = v0.x;
	int scanlineY;
	int i;
	int end;
	outp(SC_INDEX, MAP_MASK);
	for (scanlineY = v0.y; scanlineY <= v1.y; scanlineY++) {
		if (scanlineY > 200) {break;}
		if (scanlineY < 0) {
			curx1 += invslope1;
			curx2 += invslope2;
			continue;
		}
		i = curx1 * middleisleft + curx2 * (1-middleisleft);
		end = curx2 * middleisleft + curx1 * (1-middleisleft);
		if (i < 0) {i = 0;}
		if (end > 318) {end = 318;}
		while (i-1 < end) {
			
			i++;
			outp(SC_DATA,  1 << (i&3) );
			VGA[non_visible_page + (scanlineY<<6)+(scanlineY<<4)+(i>>2)]=0x00;
		}
		curx1 += invslope1;
		curx2 += invslope2;
	}
}
void draw_topflat_triangle(ivec3 v0, ivec3 v1, ivec3 v2, int middleisleft) {
	float invslope1 = ((float)v2.x - (float)v0.x) / ((float)v2.y - (float)v0.y);
	float invslope2 = ((float)v2.x - (float)v1.x) / ((float)v2.y - (float)v1.y);
	line ln;
	float curx1 = v2.x;
	float curx2 = v2.x;
	int scanlineY;
	int i;
	int end;
	outp(SC_INDEX, MAP_MASK);
	
	for (scanlineY = v2.y; scanlineY > v0.y; scanlineY--) {
		if (scanlineY > 200) {
			curx1 -= invslope1;
			curx2 -= invslope2;
			continue;
		}
		if (scanlineY < 0) {
			break;
		}
		i = curx1 * middleisleft + curx2 * (1-middleisleft);
		end = curx2 * middleisleft + curx1 * (1-middleisleft);
		if (i < 0) {i = 0;}
		if (end > 318) {end = 318;}
		while (i <= end) {
			i++;
			outp(SC_DATA,  1 << (i&3) );
			VGA[non_visible_page + (scanlineY<<6)+(scanlineY<<4)+(i>>2)]=0x00;
		}

		curx1 -= invslope1;
		curx2 -= invslope2;
	}
}
void draw_triangle3(triangle tri) {
	ivec3 v0;
	ivec3 v1;
	ivec3 v2;


	//sorted
	ivec3 v0_s;
	ivec3 v1_s;
	ivec3 v2_s;

	ivec3 v3;
	byte middleisleft = 1;

	v0.x = (int)tri.v[0].x;
	v0.y = (int)tri.v[0].y;
	
	v1.x = (int)tri.v[1].x;
	v1.y = (int)tri.v[1].y;

	v2.x = (int)tri.v[2].x;
	v2.y = (int)tri.v[2].y;


	//"sorting algorithm"
	if (v0.y < v1.y) {
		if (v2.y < v0.y) {
			v0_s = v2;
			v1_s = v0;
			v2_s = v1;
			middleisleft = 1;
		}else {
			v0_s = v0;
			if (v1.y < v2.y) {
				v1_s = v1;
				v2_s = v2;
				middleisleft = 1;
			}else {
				v1_s = v2;
				v2_s = v1;
				middleisleft = 0;
			}
		}
	}else {
		if (v2.y < v1.y) {
			v0_s = v2;
			v1_s = v1;
			v2_s = v0;
			middleisleft = 0;
		}else {
			v0_s = v1;
			if (v0.y < v2.y) {
				v1_s = v0;
				v2_s = v2;
				middleisleft = 0;
			}else {
				v1_s = v2;
				v2_s = v0;
				middleisleft = 1;
			}
		}
	}
	if (v1_s.y == v2_s.y) {
		draw_bottomflat_triangle(v0_s, v1_s, v2_s, middleisleft);
	}else if (v0_s.y == v1_s.y) {
		draw_topflat_triangle(v0_s, v1_s, v2_s, 1-middleisleft);
	}else {
		v3.x = v0_s.x + ((float)(v1_s.y - v0_s.y) / (float)(v2_s.y - v0_s.y)) * (v2_s.x - v0_s.x);
		v3.y = v1_s.y;
	
		draw_bottomflat_triangle(v0_s, v1_s, v3, middleisleft);
		draw_topflat_triangle(v1_s, v3, v2_s, middleisleft);

	}
	
}

void draw_triangle2(triangle tri) {
	ivec3 v0;
	ivec3 v1;
	ivec3 v2;
  
	int x,y;
	
	int w0;
	int w1;
	int w2;
	
	int w0_row;
	int w1_row;
	int w2_row;
	
	int A01;
	int A12;
	int A20;
	
	int B01;
	int B12;
	int B20;
	
	ivec3 point;
	
	ivec3 max = {0, 0, 0};
	ivec3 min = {320, 240, 0};


	v0.x = (int)tri.v[0].x;
	v0.y = (int)tri.v[0].y;
	v0.z = (int)tri.v[0].z;
	
	v1.x = (int)tri.v[1].x;
	v1.y = (int)tri.v[1].y;
	v1.z = (int)tri.v[1].z;

	v2.x = (int)tri.v[2].x;
	v2.y = (int)tri.v[2].y;
	v2.z = (int)tri.v[2].z;

  
	if (v0.x > max.x) {max.x = v0.x; }
	if (v1.x > max.x) {max.x = v1.x; }
	if (v2.x > max.x) {max.x = v2.x; }
	
	if (v0.x < min.x) {min.x = v0.x; }
	if (v1.x < min.x) {min.x = v1.x; }
	if (v2.x < min.x) {min.x = v2.x; }
	
	if (v0.y > max.y) {max.y = v0.y; }
	if (v1.y > max.y) {max.y = v1.y; }
	if (v2.y > max.y) {max.y = v2.y; }
	
	if (v0.y < min.y) {min.y = v0.y; }
	if (v1.y < min.y) {min.y = v1.y; }
	if (v2.y < min.y) {min.y = v2.y; }
	
	point.z = 0;


	w0_row = orient2d(v1, v2, min);
	w1_row = orient2d(v2, v0, min);
	w2_row = orient2d(v0, v1, min);
	
	A01 = v0.y - v1.y;
    A12 = v1.y - v2.y;
    A20 = v2.y - v0.y;
	
	B01 = v1.x - v0.x;
	B12 = v2.x - v1.x;
	B20 = v0.x - v2.x;

	for (y = min.y; y <= max.y; y++) {
		w0 = w0_row;
		w1 = w1_row;
		w2 = w2_row;
        for (x = min.x; x <= max.x; x++) {
			outp(SC_INDEX, MAP_MASK);
			outp(SC_DATA,  1 << (x&3) );
			point.x = x;
			point.y = y;
			
			if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
				VGA[non_visible_page + (y<<6)+(y<<4)+(x>>2)]=0;
			}
			w0 += A12;
			w1 += A20;
			w2 += A01;
		}
		w0_row += B12;
		w1_row += B20;
		w2_row += B01;
	}
}
void draw_char(unsigned char character, int x, int y) {
	int i = 0;
	int j;
	int k = 0;
	int temp = 0;
	for (k = 0; k < 13; k++) {
		for (j = 256; j >= 1; j /= 2) {
			i+=1;
			temp = letters[character-32][13-k] & j;
			if (temp == j) {
				set_pix_u(x+i,y+k, 0x0f);
			}
		}
		i=0;
	}
}

void draw_text(char * str, int x, int y) {
	int len = strlen(str);
	int i;
	for (i = 0; i < len; i++) {
		draw_char(str[i], x+i*9, y);
	}
}
void draw_background() {
	int x,y;
	
	outp(SC_INDEX, MAP_MASK);
	outp(SC_DATA,  0xf );

	for (x = 0; x < SCREEN_SIZE/4; x++) {
		VGA[non_visible_page+x] = 0x01;
	}
	
}

void draw_rect(int x, int y, int w, int h, byte c) {
	
	 int i, j;
	for( j = y; j < y + h; j++ ) {
		for( i = x; i < x + w; i++ ) {
			set_pix_u(i, j, c);
		}
	}

}
void blit2vga(byte far *s, int x, int y, int w, int h) {
	int i;
	byte far *src = s;
	byte far *dst = VGA + x + y * SCREEN_WIDTH;
	for (i = y; i < y + h; ++i) {
		movedata(FP_SEG(src), FP_OFF(src), FP_SEG(dst), FP_OFF(dst), w);
		src += w;
		dst += SCREEN_WIDTH;
	}
}
void blit2mem(byte far *d, int x, int y, int w, int h) {
	int i;
	byte far *src = VGA + x + y * SCREEN_WIDTH;
	byte far *dst = d;
	for (i = y; i < y + h; ++i) {
		movedata(FP_SEG(src), FP_OFF(src), FP_SEG(dst), FP_OFF(dst), w);
		src += SCREEN_WIDTH;
		dst += w;
	}
}
void store_player(player *p) {
	blit2mem(p->backup, p->x, p->y, p->width, p->height);
}
void restore_player(player *p) {
	blit2vga(p->backup, p->x, p->y, p->width, p->height);
}
void draw_player(player *p) {
	draw_rect(p->x, p->y, p->width, p->height, p->color);
}
void ball_wall_collide(player *ball) {
	if (ball->y > 200-8 || ball->y < 1) {
		ball->dy = -ball->dy;
	}
	if (ball->x > 320-8) {
		ball->x = SCREEN_WIDTH/2;
		ball->dx = -ball->dx;
	}
	if (ball->x < 1) {
		ball->x = SCREEN_WIDTH/2;
		ball->dx = -ball->dx;
	}
}
void ball_paddle_collide(player *ball, player *p1) {
	if (ball->x-1 < p1->x + p1->width+1 && ball->x + ball->width+1 > p1->x + 1 &&
		ball->y-1 < p1->y + p1->height+1 && ball->y + ball->height+1 > p1->y + 1) {
		ball->dx = -ball->dx;
		ball->x += ball->dx * 4;
	}
}
void handle_ball(player *p, player * p2, player *ball) {
	ball_wall_collide(ball);
	ball_paddle_collide(ball, p);
	ball_paddle_collide(ball, p2);

	if (ball->dx == 0 && ball->dy == 0) {
		ball->dx = 1;
		ball->dy = 1;
	}
	
	ball->x = ball->x + ball->dx;
	ball->y = ball->y + ball->dy;
}
byte *get_sky_palette() {
	byte *pal;
	int i;
	pal = malloc(NUM_COLORS * 3); /* 3 channels */
	
	for (i = 0; i < 100; ++i) {
		pal[i*3 + 0] = MIN(63, i); /* RED */
		pal[i*3 + 1] = MIN(63, i); /* GREEN */
		pal[i*3 + 2] = 63; /* BLUE */
	}
	for (i = 100; i < 200; ++i) {
		pal[i*3 + 0] = 5; /* RED */
		pal[i*3 + 1] = (i - 100) / 2; /* GREEN */
		pal[i*3 + 2] = 5; /* BLUE */
	}
	return pal;
}

void set_palette(byte * palette) {
	int i;
	outp(PALETTE_INDEX, 0);
	for (i = 0; i < NUM_COLORS * 3; ++i) {
		outp(PALETTE_DATA, palette[i]);
	}
}

int mx, my, mleft, mright;
static byte mcolor = WHITE;



int main() {
	char kc = 0;
	int x;
	int y;
	unsigned char val;
	
	int frames = 0;
	char str[50];
	vec3 tempvec;
	triangle square[] = {
						{{{-1, -1, 0}, {-1, 1, 0}, {1, 1, 0}}},
						{{{-1, -1, 0}, {1, 1, 0}, {1, -1, 0}}}
						};
    triangle cube[] = {
                        //front
						{{{1, 1, 1}, {-1, 1, 1}, {-1, -1, 1}}},
						{{{1, 1, 1}, {-1, -1, 1}, {1, -1, 1}}},

                        //back
                        {{{-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}}},
						{{{-1, -1, -1}, {1, 1, -1}, {1, -1, -1}}},

                        //top
                        {{{-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}}},
						{{{-1, 1, -1}, {1, 1, 1}, {1, 1, -1}}},

                        //bottom
                        {{{1, -1, 1}, {-1, -1, 1}, {-1, -1, -1}}},
						{{{1, -1, 1}, {-1, -1, -1}, {1, -1, -1}}},

                        //left
                        {{{1, 1, 1}, {1, -1, 1}, {1, -1, -1}}},
						{{{1, 1, 1}, {1, -1, -1}, {1, 1, -1}}},

                        //right
                        {{{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}}},
						{{{-1, -1, -1}, {-1, 1, 1}, {-1, 1, -1}}},


						};
	triangle tri = {{{-1, -1, 0}, {0, 1, 0}, {1, -1, 0}}};
	triangle triTrans = {{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}}};


	vec2 polygonTransform[] = {{0,0}, {0,0}, {0,0}, {0, 0}};

	vec3 pvel = {0, 0, 0};
	
	vec3 poffset = {0, 0, 0};
	vec3 pscale = {0.8,0.8,0.8};
	
	line ln = {{100, 50}, {100, 100}, 0x47};
	player p1, p2, ball;

	
	p1.color = 0x30;
	p1.height = 32;
	p1.width = 4;
	p1.x = 5;
	p1.y = SCREEN_HEIGHT / 2 - p1.height / 2;
	p1.score = 0;
	p1.backup = malloc( p1.height * p1.width );
	
	p2.color = 0x27;
	p2.height = 32;
	p2.width = 4;
	p2.x = 280;
	p2.y = SCREEN_HEIGHT / 2 - p2.height / 2;
	p2.score = 0;
	p2.backup = malloc( p2.height * p2.width );

	
	ball.color = 0x50;
	ball.height = 8;
	ball.width = 8;
	ball.x = SCREEN_WIDTH / 2 - ball.width / 2 + 80;
	ball.y = SCREEN_HEIGHT / 2 - ball.height / 2;
	ball.score = 0;
	ball.backup = malloc( ball.height * ball.width );
	
	memset( p1.backup, 0, p1.height * p1.width );
	memset( p2.backup, 0, p2.height * p2.width );

	memset( ball.backup, 0, ball.height * ball.width );

	if (!init_mouse()) {
		printf("no mouse found\n");
		return 1;
	}	
	set_mode_x ();

	
	p1.dx = 0;
	p1.dy = 0;
	p2.dx = 0;
	p2.dy = 0;
	store_player(&p1);
	store_player(&p2);
	store_player(&ball);
	ball.dx = 0;
	ball.dy = 0;
	
	/*draw_background();*/
	
	poffset.x = 0.0;
	poffset.y = 0.0;
	poffset.z = 3.0;
	
	while(kc != 0x1b) {
		
		draw_background();
		frames++;
		p1.x += 8;

		for (y = 0; y < 12; y++) {
			for (x = 0; x < 3; x++) {
				tempvec = cube[y].v[x];
				tempvec = vec3_mul(tempvec, pscale);
				tempvec = rotatey(tempvec, frames * 2 * (3.14/180.0));
				tempvec = vec3_add(tempvec, poffset); /*location transform */
				tempvec = perspective(tempvec);
		
				tempvec = toScreenspace(tempvec);
			
			
				
				triTrans.v[x] = tempvec;
			
			

			}
			draw_triangle3(triTrans);
		}
		
		if (kbhit()) { /* escape */
			kc = getch();
			if (kc == 0x77) { /*w*/
				pvel.y = -0.4;
			}
			else if (kc == 0x61) { /*a*/
				pvel.x = -0.4;
			}
			else if (kc == 0x73) { /*s*/
				pvel.y = 0.4;
			}
			else if (kc == 0x64) { /*d*/
				pvel.x = 0.4;
			}
			else if (kc == 0x65) { /*e*/
				pvel.z = 0.4;
			}
			else if (kc == 0x63) { /*e*/
				pvel.z = -0.4;
			}else {
				pvel.x = 0;
				pvel.y = 0;
				pvel.z = 0;
			}
		}else {
			pvel.x = 0;
			pvel.y = 0;
			pvel.z = 0;
		}
		poffset = vec3_add(poffset, pvel);
		page_flip();
	}
	hide_mouse();
	set_mode(TEXT_MODE);
	return 0;
}