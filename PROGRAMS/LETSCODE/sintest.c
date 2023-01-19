#include <stdio.h>
#include <math.h>

#define PI 3.14159265


 int main() {
	double dx = (double) 90;
	double yval = dx*(3.14/180.0);
	int iyval;
	yval = sin(yval) * (180.0/3.14) + 120;
	iyval = (int)yval;
	printf("sin(90)=%i\n", iyval);
	return 0;
 }