#include "math2.h"

vec2 vec2_add(vec2 v0, vec2 v1) {
	vec2 result = {0,0};
	result.x = v0.x + v1.x;
	result.y = v0.y + v1.y;
	return result;
}
vec2 vec2_sub(vec2 v0, vec2 v1) {
	vec2 result = {0,0};
	result.x = v0.x - v1.x;
	result.y = v0.y - v1.y;
	return result;
}
vec2 vec2_mul(vec2 v0, vec2 v1) {
	vec2 result = {0,0};
	result.x = v0.x * v1.x;
	result.y = v0.y * v1.y;
	return result;
}
vec2 vec2_div(vec2 v0, vec2 v1) {
	vec2 result = {0,0};
	result.x = v0.x / v1.x;
	result.y = v0.y / v1.y;
	return result;
}

vec3 vec3_add(vec3 v0, vec3 v1) {
	vec3 result = {0, 0, 0};
	result.x = v0.x + v1.x;
	result.y = v0.y + v1.y;
	result.z = v0.z + v1.z;
	return result;
}
vec3 vec3_sub(vec3 v0, vec3 v1) {
	vec3 result = {0, 0, 0};
	result.x = v0.x - v1.x;
	result.y = v0.y - v1.y;
	result.z = v0.z - v1.z;
	return result;
}
vec3 vec3_mul(vec3 v0, vec3 v1) {
	vec3 result = {0, 0, 0};
	result.x = v0.x * v1.x;
	result.y = v0.y * v1.y;
	result.z = v0.z * v1.z;
	return result;
}
vec3 vec3_div(vec3 v0, vec3 v1) {
	vec3 result = {0, 0, 0};
	result.x = v0.x / v1.x;
	result.y = v0.y / v1.y;
	result.z = v0.z / v1.z;
	return result;
}

vec3 vec2tovec3(vec2 v) {
	vec3 v2;
	v2.x = v.x;
	v2.y = v.y;
	v2.z = 0;
	return v2;
}
vec2 vec3tovec2(vec3 v) {
	vec2 v2;
	v2.x = v.x;
	v2.y = v.y;
	return v2;
}

vec3 rotatex(vec3 v, float theta) {
	vec3 out;
	out.x = v.x;
	out.y = cos(theta) * v.y - sin(theta) * v.z;
	out.z = sin(theta) * v.y + cos(theta) * v.z;
	return out;
}
vec3 rotatey(vec3 v, float theta) {
	vec3 out;
	out.x = cos(theta) * v.x + sin(theta) * v.z;
	out.y = v.y;
	out.z = -sin(theta) * v.x + cos(theta) * v.z;
	return out;
}
vec3 rotatez(vec3 v, float theta) {
	vec3 out;
	out.x = cos(theta) * v.x - sin(theta) * v.y;
	out.y = sin(theta) * v.x + cos(theta) * v.y;
	out.z = v.z;
	return out;
}
vec3 vec3_cross(vec3 v0, vec3 v1) {
	vec3 out;
	out.x = v0.y * v1.z - v0.z * v1.y;
	out.y = v0.z * v1.x - v0.x * v1.z;
	out.z = v0.x * v1.y - v0.y * v1.x;
	return out;
}
vec3 mat4_mul(vec3 v, mat4 m) {
	vec3 o;
	float w;
	o.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	o.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	o.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	
	w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
	
	if (w != 0.0f) {
		o.x /= w;
		o.y /= w;
		o.z /= w;
	}
	return o;
}

vec3 perspective(vec3 v) {
	
	float fnear = 1.0f;
	float ffar = 1000.0f;
	float fFov = 90.0f;
	float aspectratio = 240.0/320.0;
	float fFovRad = 1.0f / tan(fFov * 0.5 / (180.0f/3.1415));
	vec3 clippos;
	float clipw;
	clippos = v;
	clippos.z *= (fnear + ffar) / (fnear - ffar);
	clippos.z += 2.0f * fnear * ffar / (fnear - ffar);	
	clipw = v.z;
	if (clipw > 0.0f) {
		clippos.x /= clipw;
		clippos.y /= clipw;
	}
	
	return clippos;
}
vec3 perspectiveCheap(vec3 v) {
	vec3 out = v;
	
	if (v.z != 0.5f) {
		out.x /= v.z;
		out.y /= v.z;
	}
	
	return out;
}

vec3 toScreenspace(vec3 v) {	
	
	vec3 out = v;

	
	out.x *= 160.0f;
	out.y *= 160.0f;

	out.x += 160.0f;
	out.y += 120.0f;


	
	
	return out;
}