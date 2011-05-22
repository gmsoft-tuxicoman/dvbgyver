/*
 *  feedhunter : Automated satellite feed hunter
 *  Copyright (C) 2011 Guy Martin <gmsoft@tuxicoman.be>
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

#include "frontend.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int frontend_open(char *frontend, struct dvb_frontend_info *fe_info) {
	// Open the DVB device

	int frontend_fd = open(frontend, O_RDWR);

	if (frontend_fd == -1) {
		perror("Error while opening frontend");
		return -1;
	}
	printf("Frontend %s opened\n", frontend);

	// Get frontend info
	memset(fe_info, 0, sizeof(struct dvb_frontend_info));

	if (ioctl(frontend_fd, FE_GET_INFO, fe_info)) {
		perror("Unable to query frontend info");
		goto err;
	}

	if (fe_info->type != FE_QPSK) {
		printf("Error : the frontend %s is not a DVB-S frontend !\n", frontend);
		goto err;
	}

	if (!fe_info->frequency_stepsize) {
		printf("Warning, frontend returned frequency step size of 0, defaulting to 1Mhz\n");
		fe_info->frequency_stepsize = 1000;
	}

	return frontend_fd;

err:
	close(frontend_fd);
	return -1;
}

void frontend_print_info(struct dvb_frontend_info *fe_info) {

	printf("Frontend info :\n"
		"  Mininum frequency   : %u Mhz\n"
		"  Maximum frequency   : %u Mhz\n"
		"  Frequency step      : %u Mhz\n"
		"  Mininum symbol rate : %u MSym/S\n"
		"  Maximum symbol rate : %u MSym/S\n"
		"  Can do DVB-S2       : %s\n"

		, fe_info->frequency_min / 1000
		, fe_info->frequency_max / 1000
		, fe_info->frequency_stepsize / 1000
		, fe_info->symbol_rate_min / 1000000
		, fe_info->symbol_rate_max / 1000000
		, ((fe_info->caps & FE_CAN_2G_MODULATION) ? "yes" : "no")
		);

}

int frontend_tune(int frontend_fd, unsigned int ifreq, unsigned int symbol_rate, unsigned int timeout) {

	struct dvb_frontend_parameters params;
	struct dvb_frontend_event ev;

	params.frequency = ifreq;
	params.inversion = INVERSION_AUTO;
	params.u.qpsk.symbol_rate = symbol_rate;
	params.u.qpsk.fec_inner = FEC_AUTO;

	if (ioctl(frontend_fd, FE_SET_FRONTEND, &params)) {
		perror("Error while setting frontend");
		return -1;
	}

	return 0;
}

int frontend_close(int frontend_fd) {
	
	return close(frontend_fd);
}
