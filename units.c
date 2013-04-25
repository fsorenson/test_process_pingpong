#include "units.h"
#include "common.h"

#include <stdio.h>
#include <math.h>


char *subsec_string(char *arg_buffer, long double size, int dec_pts) {
	static char local_buffer[1024];
	char *buffer;
	unsigned int mult = 0;
	int p = 0, unit_p = 0;

	const char *unit_strings_sub[] = { "s", "ms", "us", "ns", "ps", "fs" };
	unsigned int max_mult = sizeof(unit_strings_sub) / sizeof(unit_strings_sub[0]);

	buffer = (arg_buffer != NULL) ? arg_buffer : local_buffer;

	if (size <= 0.0) /* don't do negatives...  I said so. */
		mult = max_mult;
	if (size == INFINITY) /* don't do infinity either. */
		mult = max_mult;

	while ((size < 1.0) && (mult < max_mult)) {
		mult ++;
		size *= 1000.0;
	}
	if (mult >= max_mult) {
		size = 0.0;
		mult = 0;
	}

	if (size >= 100.0)
		buffer[p++] = (char)(((int)(size / 100.0) % 10) + '0');
	if (size >= 10.0)
		buffer[p++] = (char)(((int)(size / 10.0) % 10) + '0');
	buffer[p++] = (char)(((int)(size) % 10) + '0');

	if (dec_pts > 0) {
		buffer[p++] = '.';

		size -= truncl(size);
		while (dec_pts-- > 0) {
			size *= 10.0;
			buffer[p++] = (char)(size + '0');
			size -= truncl(size);
		}
	}

	buffer[p++] = ' ';
	unit_p = 0;
	while (unit_strings_sub[mult][unit_p] != 0x00)
		buffer[p++] = unit_strings_sub[mult][unit_p++];
	buffer[p++] = 0x00;

	return buffer;
}

typedef struct integer_time {
        int s;
        int ms;
} integer_time_t;

integer_fixed_point_t __CONST f_to_fp(int prec, double f) {
        integer_fixed_point_t int_fp;

        int_fp.prec = prec;
        int_fp.i = (unsigned long int) f;
        int_fp.dec = (unsigned long)((f - (double)int_fp.i) * prec);

        return int_fp;
}

