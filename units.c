/*
	This file is part of test_process_pingpong

	Copyright 2013 Frank Sorenson (frank@tuxrocks.com)


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "units.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


char *subsec_string(char *arg_buffer, long double size, int dec_pts) {
	integer_fixed_point_t int_fp;


	static char local_buffer[32];
	char *buffer;
	unsigned int mult = 0;
	int p = 0, unit_p = 0;

	const char *unit_strings_sub[] = { "s", "ms", "us", "ns", "ps", "fs" };
	unsigned int max_mult = sizeof(unit_strings_sub) / sizeof(unit_strings_sub[0]);

	buffer = (arg_buffer != NULL) ? arg_buffer : local_buffer;

	if (size <= 0.0L) /* don't do negatives...  I said so. */
		mult = max_mult;

	int_fp = f_to_fp(dec_pts, size);

	while ((size < 1.0L) && (mult < max_mult)) {
		mult ++;
		size *= 1000.0L;
	}
	if (mult >= max_mult) {
		size = 0.0L;
		mult = 0;
	}

	int_fp = f_to_fp(dec_pts, size);


	if (int_fp.i >= 100)
		buffer[p++] = (char)(((int_fp.i / 100) % 10) + '0');
	if (size >= 10)
		buffer[p++] = (char)(((int_fp.i / 10) % 10) + '0');
	buffer[p++] = (char)((int_fp.i % 10) + '0');

	if (dec_pts > 0) {
//		i = mult
		buffer[p++] = '.';

		size -= truncl(size);
		while (dec_pts-- > 0) {
			size *= 10.0L;
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

