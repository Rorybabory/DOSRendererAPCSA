#ifndef _MATH_H_
#define _MATH_H_
#include "types.h"
#include <math.h>

vec2 vec2_add(vec2 v0, vec2 v1);
vec2 vec2_sub(vec2 v0, vec2 v1);
vec2 vec2_mul(vec2 v0, vec2 v1);
vec2 vec2_div(vec2 v0, vec2 v1);

vec3 vec3_add(vec3 v0, vec3 v1);
vec3 vec3_sub(vec3 v0, vec3 v1);
vec3 vec3_mul(vec3 v0, vec3 v1);
vec3 vec3_div(vec3 v0, vec3 v1);

vec3 vec3_cross(vec3 v0, vec3 v1);

vec3 vec2tovec3(vec2 v);
vec2 vec3tovec2(vec3 v);

vec3 rotatex(vec3 v, float theta);
vec3 rotatey(vec3 v, float theta);
vec3 rotatez(vec3 v, float theta);

vec3 mat4_mul(vec3 v, mat4 m);


vec3 perspectiveCheap(vec3 v);
vec3 perspective(vec3 v);

vec3 toScreenspace(vec3 v);

#endif