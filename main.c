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
#include <getopt.h>
#include <limits.h>

#include "frontend.h"

void print_usage(char *app) {

	printf("Usage : %s <options>\n"
		"\n"
		"Options are :\n"
		" -a, --adapter=X        Adapter to use\n"
		" -f, --frontend=X       Frontend to use\n"
		,app);

}

int main(int argc, char *argv[]) {

	
	// Parse command line
	unsigned int adapter = 0;
	unsigned int frontend = 0;



	while (1) {

		static struct option long_options[] = {
			{ "adapter", 1, 0, 'a' },
			{ "frontend", 1, 0, 'f' },

		};

		char *args = "a:f:";

		int c = getopt_long(argc, argv, args, long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'a':
				if (sscanf(optarg, "%u", &adapter) != 1) {
					printf("Invalid adapter \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'f':
				if (sscanf(optarg, "%u", &frontend) != 1) {
					printf("Invalid frontend \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			default:
				print_usage(argv[0]);
				return 1;
		}

	}

	// Open the DVB device

	char frontend_str[NAME_MAX];
	snprintf(frontend_str, NAME_MAX - 1, "/dev/dvb/adapter%u/frontend%u", adapter, frontend);

	struct dvb_frontend_info fe_info;

	int frontend_fd = frontend_open(frontend_str, &fe_info);
	if (frontend_fd == -1)
		return 1;

	frontend_print_info(&fe_info);


	frontend_close(frontend_fd);

	return 0;
}

