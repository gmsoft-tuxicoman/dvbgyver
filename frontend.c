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
#include "utils.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

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

	return frontend_fd;

err:
	close(frontend_fd);
	return -1;
}

void frontend_print_info(struct dvb_frontend_info *fe_info) {

	char *fe_type = "Unknown";
	switch (fe_info->type) {
		case FE_QPSK:
			fe_type = "DVB-S";
			break;
		case FE_QAM:
			fe_type = "DVB-C";
			break;
		case FE_OFDM:
			fe_type = "DVB-T";
			break;
		case FE_ATSC:
			fe_type = "ATSC";
			break;
	}
	

	printf("Frontend info :\n"
		"  Frontend type       : %s\n"
		"  Mininum frequency   : %u Mhz\n"
		"  Maximum frequency   : %u Mhz\n"
		"  Frequency step      : %u Mhz\n"
		"  Mininum symbol rate : %u MSym/S\n"
		"  Maximum symbol rate : %u MSym/S\n"
		, fe_type
		, fe_info->frequency_min / 1000
		, fe_info->frequency_max / 1000
		, fe_info->frequency_stepsize / 1000
		, fe_info->symbol_rate_min / 1000000
		, fe_info->symbol_rate_max / 1000000);

	if (fe_info->type == FE_QPSK) {
		printf ("  Can do DVB-S2       : %s\n"
			, ((fe_info->caps & FE_CAN_2G_MODULATION) ? "yes" : "no"));
	}

}

int frontend_tune_dvb_s(int frontend_fd, unsigned int ifreq, unsigned int symbol_rate) {

	struct dvb_frontend_parameters params = {0};

	params.frequency = ifreq;
	params.inversion = INVERSION_AUTO;
	params.u.qpsk.symbol_rate = symbol_rate;
	params.u.qpsk.fec_inner = FEC_AUTO;

	if (ioctl(frontend_fd, FE_SET_FRONTEND, &params)) {
		perror("Error while setting frontend");
		return 1;
	}
	
	return 0;
}

int frontend_tune_dvb_c(int frontend_fd, unsigned int freq, unsigned int symbol_rate, fe_modulation_t modulation) {
	
	struct dvb_frontend_parameters params = {0};
	params.frequency = freq;
	params.inversion = INVERSION_AUTO;
	params.u.qam.symbol_rate = symbol_rate;
	params.u.qam.fec_inner = FEC_AUTO;
	params.u.qam.modulation = modulation;

	if (ioctl(frontend_fd, FE_SET_FRONTEND, &params)) {
		perror("Error while setting frontend");
		return 1;
	}

	return 0;
}

int frontend_tune_dvb_t(int frontend_fd, unsigned int freq, fe_modulation_t modulation, fe_bandwidth_t bandwidth, fe_transmit_mode_t transmit_mode, fe_code_rate_t code_rate, fe_guard_interval_t guard_interval) {

	struct dvb_frontend_parameters params = {0};
	params.frequency = freq;
	params.inversion = INVERSION_AUTO;
	params.u.ofdm.constellation = modulation;
	params.u.ofdm.code_rate_HP = code_rate;
	params.u.ofdm.code_rate_LP = FEC_NONE;
	params.u.ofdm.constellation = modulation;
	params.u.ofdm.transmission_mode = transmit_mode;
	params.u.ofdm.guard_interval = guard_interval;
	params.u.ofdm.hierarchy_information = HIERARCHY_NONE; // Only this is supported now

	if (ioctl(frontend_fd, FE_SET_FRONTEND, &params)) {
		perror("Error while setting frontend");
		return 1;
	}

	return 0;
}


int frontend_get_status(int frontend_fd, unsigned int timeout, fe_status_t *status) {

	struct pollfd pfd[1];
	pfd[0].fd = frontend_fd;
	pfd[0].events = POLLIN;

	struct timeval now;
	gettimeofday(&now, NULL);
	
	time_t expiry = now.tv_sec + timeout;

	while (now.tv_sec < expiry) {

		int res = poll(pfd, 1, 1000);

		if (res < 0) {
			perror("Error while polling frontend");
			return -1;
		}

		if (!res) { // Timeout
			gettimeofday(&now);
			continue;
		}
		
		if (pfd[0].revents & POLLIN) {
			memset(status, 0, sizeof(fe_status_t));
			if (ioctl(frontend_fd, FE_READ_STATUS, status)) {
				perror("Error while getting frontend status");
				return -1;
			}

			dvb_debug("Status : ");
			if (*status & FE_HAS_SIGNAL)
				dvb_debug("SIGNAL ");
			if (*status & FE_HAS_CARRIER)
				dvb_debug("CARRIER ");
			if (*status & FE_HAS_VITERBI)
				dvb_debug("VITERBI ");
			if (*status & FE_HAS_SYNC)
				dvb_debug("SYNC ");
			if (*status & FE_HAS_LOCK)
				dvb_debug("LOCK");
			dvb_debug("\n");

			if (*status & FE_HAS_LOCK) {
				// Got lock
				return 0;
			}

		}
		gettimeofday(&now, NULL);
	}

	return 0;
}

int frontend_set_voltage(int frontend_fd, fe_sec_voltage_t v) {

	if (ioctl(frontend_fd, FE_SET_VOLTAGE, v)) {
		perror("Error while setting voltage");
		return -1;
	}

	return 0;
}

int frontend_set_tone(int frontend_fd, fe_sec_tone_mode_t t) {

	if (ioctl(frontend_fd, FE_SET_TONE, t)) {
		perror("Error while setting tone");
		return -1;
	}

	return 0;
}

int frontend_close(int frontend_fd) {
	
	return close(frontend_fd);
}
