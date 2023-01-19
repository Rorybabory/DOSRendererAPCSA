#include <alloc.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#define NUM_COLORS 256
#define SET_MODE 0x0
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03
#define SCREEN_HEIGHT 200
#define SCREEN_WIDTH 320
#define PALETTE_INDEX 0x3c8
#define PALETTE_DATA 0x3c9
#define INPUT_STATUS 0x3DA
#define VRETRACE_BIT 0x08


/* Mouse */
#define MOUSE_INT 0x33
#define INIT_MOUSE 0x00
#define SHOW_MOUSE 0x01
#define HIDE_MOUSE 0x02
#define GET_MOUSE_STATUS 0x03

#define BLACK 0x00;
#define WHITE 0x0F;

typedef unsigned char byte;


typedef struct {
  byte color;
  int x, y;
  int dx, dy;
  int width;
  int height;
  int score;
  byte *backup;
} player;

byte far *VGA = (byte far *)(0xA0000000L);

#define SETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)=c
#define GETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))


void wait_for_retrace() {
	while( inp( INPUT_STATUS ) & VRETRACE_BIT );
	while( ! (inp( INPUT_STATUS ) & VRETRACE_BIT) );
}

void set_mode(byte mode) {
	union REGS regs;
	regs.h.ah = SET_MODE;
	regs.h.al = mode;
	int86(VIDEO_INT, &regs, &regs);
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

void draw_background() {
	int x,y;
	
	for (y = 0; y < SCREEN_HEIGHT; ++y) {
		for (x = 0; x < SCREEN_WIDTH; ++x) {
			if (x > SCREEN_WIDTH/2-3 && x < SCREEN_WIDTH/2+3 &&
				y % 48 < 24) {
				SETPIX(x, y, 0x0F);
			}else {
				SETPIX(x, y, 0x00);
			}
			
		}
	}
}
void draw_rect(int x, int y, int w, int h, byte c) {
	  int i, j;
	for( j = y; j < y + h; ++j ) {
		for( i = x; i < x + w; ++i ) {
			byte far *dst = VGA + i + j * SCREEN_WIDTH;
			*dst = c;
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

	set_mode (VGA_256_COLOR_MODE);

	draw_background();
	p1.dx = 0;
	p1.dy = 0;
	p2.dx = 0;
	p2.dy = 0;
	store_player(&p1);
	store_player(&p2);
	store_player(&ball);
	ball.dx = 0;
	ball.dy = 0;
	
	
	while(kc != 0x1b) {
		wait_for_retrace();
		restore_player(&p1);
		restore_player(&p2);
		
		restore_player(&ball);
		
		get_mouse(&mx, &my, &mleft, &mright);
		if (mleft) {
			SETPIX(mx, my, mcolor);
		}
		if (mright) {
			SETPIX(mx, my, BLACK);
		}
		
		if (kbhit()) { /* escape */
			kc = getch();
			if (kc == 0x77) { /*w*/
				p1.dy = -4;
			}
			else if (kc == 0x61) { /*a*/
				p1.dx = -4;
			}
			else if (kc == 0x73) { /*s*/
				p1.dy = 4;
			}
			else if (kc == 0x64) { /*d*/
				p1.dx = 4;
			}
		}else {
			p1.dx = 0;
			p1.dy = 0;
		}
		if (mx < SCREEN_WIDTH/2) {
			p1.x = mx;
			p1.y = my;
		}
		
		handle_ball(&p1, &p2, &ball);
		if (ball.dx > 0) {
			if (ball.y > p2.y+17) {
				p2.y += 1;
			}else if (ball.y < p2.y+15) {
				p2.y -= 1;
			}
			if (ball.x < p2.x) {
				p2.x -= 1;
			}else {
				
			}
		}else {
			if (p2.x < 290) {
				p2.x += 1;
			}
		}
		
		
		store_player(&p1);
		store_player(&p2);
		store_player(&ball);
		draw_player(&p1);
		draw_player(&p2);
		draw_player(&ball);
	}
	hide_mouse();
	set_mode(TEXT_MODE);
	return 0;
}