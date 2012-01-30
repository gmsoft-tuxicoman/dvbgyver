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


#include "lnb.h"


static struct lnb_parameters lnbs[] = {
	{ "universal", 9750000, 10600000, 11700000, 10700000, 12750000 },
};

int lnb_get_parameters(enum lnb_type type, unsigned int frequency, unsigned int *ifreq, unsigned int *hiband) {

	if (!ifreq || !hiband)
		return -1;

	*hiband = 0;

	struct lnb_parameters *lnb = &lnbs[type];

	if (frequency > lnb->switch_val)
		*hiband = 1;

	if (*hiband) {
		*ifreq = frequency - lnb->high_val;
	} else {
		if (frequency < lnb->low_val)
			*ifreq = lnb->low_val - frequency;
		else
			*ifreq = frequency - lnb->low_val;
	}

	return 0;
}

int lnb_get_limits(enum lnb_type type, unsigned int *min_freq, unsigned int *max_freq) {

	if (!min_freq || !max_freq)
		return -1;
	
	*min_freq = lnbs[type].min_freq;
	*max_freq = lnbs[type].max_freq;

	return 0;
	
}

