/*
 *  dvbgyver: A suite of tools for DVB 
 *  Copyright (C) 2012 Guy Martin <gmsoft@tuxicoman.be>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <stdarg.h>
#include <stdio.h>

#include "utils.h"

static int dvb_verbose = 0;

void dvb_set_verbose(int verbose) {
	dvb_verbose = verbose;
}

int dvb_get_verbose() {
	return dvb_verbose;
}

int dvb_debug(const char *format, ...) {
	if (!dvb_verbose)
		return 0;

	int ret = 0;

	va_list arg_list;
	va_start(arg_list, format);
	ret = vprintf(format, arg_list);
	va_end(arg_list);

	return ret;
}
