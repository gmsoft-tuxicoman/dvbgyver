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


#ifndef __LNB_H__
#define __LNB_H__

enum lnb_type {
	lnb_type_univeral,
};

struct lnb_parameters {
	char *name;
	unsigned int low_val;
	unsigned int high_val;
	unsigned int switch_val;
	unsigned int min_freq;
	unsigned int max_freq;
};

int lnb_get_parameters(enum lnb_type type, unsigned int frequency, unsigned int *ifreq, unsigned int *hiband);
int lnb_get_limits(enum lnb_type, unsigned int *min_freq, unsigned int *max_freq);



#endif
