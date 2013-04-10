#ifndef __UNITS_H__
#define __UNITS_H__


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

char *subsec_string(char *arg_buffer, long double size, int dec_pts);

typedef struct integer_fixed_point {
	unsigned long int i;
	unsigned long int dec;
	int prec;
	char dummy[4];
} integer_fixed_point_t;

integer_fixed_point_t f_to_fp(int prec, double f);


#endif
