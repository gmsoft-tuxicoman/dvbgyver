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

#ifndef __FRONTEND_H__
#define __FRONTEND_H__

#include <linux/dvb/frontend.h>

int frontend_open(char *frontend, struct dvb_frontend_info *fe_info);
void frontend_print_info(struct dvb_frontend_info *fe_info);
int frontend_tune(int frontend_fd, unsigned int ifreq, unsigned int symbol_rate, unsigned int timeout);
int frontend_close(int frontend_fd);

#endif
