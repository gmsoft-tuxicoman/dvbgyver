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

#include <stdio.h>

#include "scan.h"
#include "frontend.h"
#include "lnb.h"
#include "utils.h"

int scan(int frontend_fd, unsigned int timeout, unsigned int start_freq, unsigned int end_freq, unsigned int step) {

	unsigned int cur_freq = start_freq;
	unsigned int sample_rate = 27500000;

	printf("Scanning from %u Mhz to %u Mhz with %u Mhz steps ...\n", start_freq / 1000, end_freq / 1000, step / 1000);

	int polarity = 0;
	int polarity_try = 0;

	while (cur_freq <= end_freq) {


		scan_progress(cur_freq - start_freq, end_freq - start_freq);

		unsigned int ifreq = 0, hiband = 0;
		if (lnb_get_parameters(lnb_type_univeral, cur_freq, &ifreq, &hiband)) {
			printf("Error while getting LNB parameters\n");
			return -1;
		}


		dvb_debug("Tuning to %u Mhz, %u MSym/s, %s Polarity ...\n", cur_freq / 1000, sample_rate / 1000, (polarity ? "V" : "H"));
		// 13V is vertical polarity and 18V is horizontal
		if (frontend_set_voltage(frontend_fd, (polarity ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18)))
			return -1;

		if (frontend_set_tone(frontend_fd, (hiband ? SEC_TONE_ON : SEC_TONE_OFF)))
			return -1;

		if (frontend_tune_dvb_s(frontend_fd, ifreq, sample_rate))
			return -1;

		fe_status_t status;
		if (frontend_get_status(frontend_fd, timeout,  &status))
			return -1;

		if (status & FE_HAS_LOCK) {
			// TODO write down we locked
			// Try next frequency with same polarity
			cur_freq += step;
		} else if (polarity_try >= 1) {
			// Go to next frequency and reset polarity
			polarity = 0;
			polarity_try = 0;
			cur_freq += step;
		} else {
			// Switch polarity
			polarity = !polarity;
			polarity_try++;
		}

	}
	
	return 0;
}


int scan_progress(unsigned int cur, unsigned int max) {

	if (dvb_get_verbose())
		return 0;

	double progress = 100.0 / (double) max * (double) cur;

	printf("\r[");

	int bars = progress / 2;
	int spaces = 50 - bars;
	int i;

	for (i = 0; i < bars; i++)
		printf("-");
	for (i = 0; i < spaces; i++)
		printf(" ");
	printf("] %0.2f%%", progress);

	fflush(NULL);
}
