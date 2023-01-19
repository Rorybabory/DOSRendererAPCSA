#ifndef _MY_TYPES_H_
#define _MY_TYPES_H_

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long  dword;


typedef struct {
  byte color;
  int x, y;
  int dx, dy;
  int width;
  int height;
  int score;
  byte *backup;
} player;



typedef struct {
	float x;
	float y;
} vec2;

typedef struct {
	float x;
	float y;
	float z;
} vec3;

typedef struct {
	int x;
	int y;
	int z;
} ivec3;


typedef struct {
	float m[4][4];
} mat4;

typedef struct {
	vec2 v0;
	vec2 v1;
	byte c;
} line;

typedef struct {
	vec3 v[4];
} shape;

typedef struct {
  vec3 v[3];
} triangle;

#endif