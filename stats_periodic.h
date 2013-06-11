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


#ifndef __STATS_PERIODIC_H__
#define __STATS_PERIODIC_H__

#include "test_process_pingpong.h"
#include "stats_common.h"

int gather_periodic_stats(struct interval_stats_struct *i_stats);
void show_periodic_stats(void);
void show_periodic_stats_header(void);
void show_periodic_stats_data(struct interval_stats_struct *i_stats);
void store_last_stats(struct interval_stats_struct *i_stats);

#endif
