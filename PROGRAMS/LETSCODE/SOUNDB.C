#include "dos.h"
#include "stdio.h"
#include "stdlib.h"
#include "mem.h"

int sb_detect();
void sb_init();
void sb_deinit();

short sb_base; /* default 220h */
char sb_irq;/* default 7 */
char sb_dma; /* default 1 */

void interrupt ( *old_irq)();

volatile int playing;
volatile long to_be_played;

unsigned char * dma_buffer;
short page;
short offset;

#define SB_RESET 0x6
#define SB_READ_DATA 0xA
#define SB_WRITE_DATA 0xC
#define SB_READ_DATA_STATUS 0xE

#define SB_ENABLE_SPEAKER 0xD1
#define SB_DISABLE_SPEAKER 0xD3
#define SB_SET_PLAYBACK_FREQENCY 0x40
#define SB_SINGLE_CYCLE_PLAYBACK 0x14

#define MASK_REGISTER 0x0A
#define MODE_REGISTER 0x0B
#define MSB_LSB_FLIP_FLOP 0x0C
#define DMA_CHANNEL_0 0x87
#define DMA_CHANNEL_1 0x83
#define DMA_CHANNEL_3 0x82


int reset_dsp (short port) {
	outportb(port + SB_RESET, 1);
	delay(10);
	outportb(port + SB_RESET, 0);
	delay(10);
	if ((inportb(port + SB_READ_DATA_STATUS) & 0x80 == 0x80) &&
		(inportb(port + SB_READ_DATA) == 0x0AA)) {
		sb_base = port;
		return 1;
	}
	return 0;
}
void write_dsp(unsigned char command) {
	while ( (inportb(sb_base + SB_WRITE_DATA) & 0x80) == 0x80);
	outportb(sb_base + SB_WRITE_DATA, command);
}
void interrupt sb_irq_handler()
{
	inportb(sb_base + SB_READ_DATA_STATUS);
	outportb(0x20, 0x20);
	if (sb_irq == 2 || sb_irq == 10 || sb_irq == 11) {
		outportb(0xA0, 0x20);
	}
	playing = 0;
}
void init_irq() {
	/* save the old irq vector*/
	if ( sb_irq == 2 || sb_irq == 10 || sb_irq == 11) {
		if (sb_irq == 2) {old_irq = getvect(0x71); }
		if (sb_irq == 10) {old_irq = getvect(0x72); }
		if (sb_irq == 11) {old_irq = getvect(0x73); }
	}else {
		old_irq = getvect(sb_irq+8);
	}
	/*set our own irq vector*/
	if ( sb_irq == 2 || sb_irq == 10 || sb_irq == 11) {
		if (sb_irq == 2) {setvect(0x71, sb_irq_handler); }
		if (sb_irq == 10) {setvect(0x72, sb_irq_handler); }
		if (sb_irq == 11) {setvect(0x73, sb_irq_handler); }
	}else {
		setvect(sb_irq + 8, sb_irq_handler);
	}
	/* enable irq with the mainboard's PIC */
	if ( sb_irq == 2 || sb_irq == 10 || sb_irq == 11) {
		if (sb_irq == 2) {outportb(0xA1, inportb(0xA1) & 253);}
		if (sb_irq == 10) {outportb(0xA1, inportb(0xA1) & 251);}
		if (sb_irq == 11) {outportb(0xA1, inportb(0xA1) & 247);}
		outportb(0x21, inportb(0x21) & 251);
	}else {
		outportb(0x21, inportb(0x21) & !(1 << sb_irq));
	}
}
void deinit_irq() {
	/* restore the old irq vector*/
	if ( sb_irq == 2 || sb_irq == 10 || sb_irq == 11) {
		if (sb_irq == 2) {setvect(0x71, old_irq); }
		if (sb_irq == 10) {setvect(0x72, old_irq); }
		if (sb_irq == 11) {setvect(0x73, old_irq); }
	}else {
		setvect(sb_irq+8, old_irq);
	}
	/* enable irq with the mainboard's PIC */
	if ( sb_irq == 2 || sb_irq == 10 || sb_irq == 11) {
		if (sb_irq == 2) {outportb(0xA1, inportb(0xA1) | 2);}
		if (sb_irq == 10) {outportb(0xA1, inportb(0xA1) | 4);}
		if (sb_irq == 11) {outportb(0xA1, inportb(0xA1) | 8);}
		outportb(0x21, inportb(0x21) | 4);
	}else {
		outportb(0x21, inportb(0x21) & (1 << sb_irq));
	}
}
void assign_dma_buffer() {
	unsigned char * temp_buf;
	long linear_address;
	short page1, page2;
	
	temp_buf = (unsigned char *) malloc(32768);
	linear_address = FP_SEG(temp_buf);
	linear_address = (linear_address << 4) + FP_OFF(temp_buf);
	page1 = linear_address >> 16;
	page2 = (linear_address + 32767) >> 16;
	if (page1 != page2) {
		dma_buffer = (char *) malloc(32768);
		free(temp_buf);
	}else {
		dma_buffer = temp_buf;
	}
	linear_address = FP_SEG(dma_buffer);
	linear_address = (linear_address << 4) + FP_OFF(dma_buffer);
	page = linear_address >> 16;
	offset = linear_address & 0xFFFF;

}

int sb_detect()
{
	char temp;
	char * BLASTER;
	sb_base = 0;
	
	/* possible values: 210, 220, 230, 240, 250, 260, 280*/
	for (temp = 1; temp < 9; temp++) {
		if (temp) {
			if (temp != 7) {
				if (reset_dsp(0x200 + temp * 0x10)) {
					break;
				}
			}
		}
	}
	if (temp == 9) {
		return 0;
	}
	BLASTER = getenv("BLASTER");
	sb_dma = 0;
	for (temp = 0; temp < strlen(BLASTER); ++temp) {
		if (((BLASTER[temp] | 32 ) == 'd')) {
			sb_dma = BLASTER[temp + 1] - '0';
		}
	}
	
	for (temp = 0; temp < strlen(BLASTER); ++temp) {
		if (((BLASTER[temp] | 32 ) == 'i')) {
			sb_irq = BLASTER[temp + 1] - '0';
			if (BLASTER[temp+2] != ' ') {
				sb_irq = sb_irq * 10 + BLASTER[temp + 2] - '0';
			}
		}
	}
	return sb_base != 0;
}

void sb_init()
{
	init_irq();
	assign_dma_buffer();
	write_dsp(SB_ENABLE_SPEAKER);
}
void single_cycle_playback() {
	playing = 1;
	outportb(MASK_REGISTER, 4 | sb_dma);
	outportb(MSB_LSB_FLIP_FLOP, 0);
	outportb(MODE_REGISTER, 0x48 | sb_dma);
	outportb(sb_dma << 1, offset & 0xFF);
	outportb(sb_dma << 1, offset >> 8);
	
	switch (sb_dma) {
		case 0:
			outportb(DMA_CHANNEL_0, page);
			break;
		case 1:
			outportb(DMA_CHANNEL_1, page);
			break;
		case 3:
			outportb(DMA_CHANNEL_3, page);
			break;
		default:
			break;
	}
	outportb((sb_dma << 1) + 1, to_be_played & 0xFF);
	outportb((sb_dma << 1) + 1, to_be_played >> 8);
	outportb(MASK_REGISTER, sb_dma);
	write_dsp(SB_SINGLE_CYCLE_PLAYBACK);
	write_dsp(to_be_played & 0xFF);
	write_dsp(to_be_played >> 8);
	to_be_played = 0;
}
void sb_single_play(const char * filename) {
	FILE * raw_file;
	int file_size;
	memset(dma_buffer, 0, 32768);
	raw_file = fopen(filename, "rb");
	fseek(raw_file, 0L, SEEK_END);
	file_size = ftell(raw_file);
	fseek(raw_file, 0L, SEEK_SET);
	fread(dma_buffer, 1, file_size, raw_file);
	write_dsp(SB_SET_PLAYBACK_FREQENCY);
	write_dsp(256-1000000/44100);
	to_be_played = file_size;
	single_cycle_playback();
	
}

void sb_deinit() {
	write_dsp(SB_DISABLE_SPEAKER);
	free(dma_buffer);
	deinit_irq();
}
int main() {
	int sb_detected = 0;
	sb_detected = sb_detect();
	if (!sb_detected) {
		printf("SOUNDBLASTER NOT FOUND!\n");
		return 0;
	}else {
		printf("SOUNDBLASTER FOUND AT A%x I%u D%u.\n", sb_base, sb_irq, sb_dma);
	}
	
	sb_init();
	
	sb_single_play("tadaa.raw");
	while (playing)
		;
	sb_deinit();
	
	return 0;
}