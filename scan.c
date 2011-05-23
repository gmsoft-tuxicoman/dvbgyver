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
#include "main.h"


int scan(int frontend_fd, unsigned int timeout, unsigned int start_freq, unsigned int end_freq, unsigned int step) {

	unsigned int cur_freq = start_freq;
	unsigned int sample_rate = 27500000;

	printf("Scanning from %u Mhz to %u Mhz with %u Mhz steps ...\n", start_freq / 1000, end_freq / 1000, step / 1000);

	for (cur_freq = start_freq; cur_freq <= end_freq; cur_freq += step) {

		fh_progress(cur_freq - start_freq, end_freq - start_freq);

		unsigned int ifreq = 0, hiband = 0;
		if (lnb_get_parameters(lnb_type_univeral, cur_freq, &ifreq, &hiband)) {
			printf("Error while getting LNB parameters\n");
			return -1;
		}

		int i;
		// Try vertical and horizontal polarization
		for (i = 0; i < 2; i++) {
			fh_debug("Tuning to %u Mhz, %u MSym/s, %s Polarity ...\n", cur_freq / 1000, sample_rate / 1000, (i ? "V" : "H"));
			// 13V is vertical polarity and 18V is horizontal
			if (frontend_set_voltage(frontend_fd, (i ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18)))
				return -1;

			if (frontend_set_tone(frontend_fd, (hiband ? SEC_TONE_ON : SEC_TONE_OFF)))
				return -1;

			if (frontend_tune(frontend_fd, ifreq, sample_rate))
				return -1;

			fe_status_t status;
			if (frontend_get_status(frontend_fd, timeout,  &status))
				return -1;

			fh_debug("Status : ");
			if (status & FE_HAS_SIGNAL)
				fh_debug("SIGNAL ");
			if (status & FE_HAS_CARRIER)
				fh_debug("CARRIER ");
			if (status & FE_HAS_VITERBI)
				fh_debug(" VITERBI ");
			if (status & FE_HAS_SYNC)
				fh_debug(" SYNC ");
			if (status & FE_HAS_LOCK)
				fh_debug(" LOCK");

			fh_debug("\n");

		}

	}

	
	return 0;
}


