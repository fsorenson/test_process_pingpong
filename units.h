#ifndef __UNITS_H__
#define __UNITS_H__

#include "common.h"


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

char *subsec_string(char *arg_buffer, long double size, int dec_pts);

integer_fixed_point_t f_to_fp(int prec, double f);


#endif
