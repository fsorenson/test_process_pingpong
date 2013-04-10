#ifndef __UNITS_H__
#define __UNITS_H__


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <math.h>

char *subsec_string(char *arg_buffer, long double size, int dec_pts);

typedef struct integer_fixed_point {
	int prec;
	unsigned long int i;
	unsigned long int dec;
} integer_fixed_point_t;

integer_fixed_point_t f_to_fp(int prec, double f);


#endif
