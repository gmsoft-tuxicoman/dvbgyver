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
#include <stdarg.h>

#include "main.h"
#include "frontend.h"
#include "lnb.h"
#include "config.h"

unsigned int verbose = 0;

void print_usage(char *app) {

	printf("Usage : %s <options>\n"
		"\n"
		"Options are :\n"
		" -h, --help             Display this help and exit\n"
		" -a, --adapter=X        Adapter to use\n"
		" -f, --frontend=X       Frontend to use\n"
		" -t, --timeout=X        Tuning timeout in seconds, default 5\n"
		" -v, --verbose          Increase verbosity\n"
		" -m, --min-freq         Lower bound of the frequency range to scan in Mhz\n"
		" -M, --max-freq         Higher bound of the frequency range to scan in Mhz\n"
		" -s, --step-freq        Frequency steps in Mhz. Default 1Mhz or higher depending on the card\n"
		"\n"
		,app);

}

int main(int argc, char *argv[]) {


	printf(PACKAGE " : Copyright " PACKAGE_BUGREPORT "\n\n");
	
	// Parse command line
	unsigned int adapter = 0;
	unsigned int frontend = 0;
	unsigned int tuning_timeout = 3;

	unsigned int freq_start = 0, freq_end = 0, freq_step = 0;


	while (1) {

		static struct option long_options[] = {
			{ "help", 0, 0, 'h' },
			{ "adapter", 1, 0, 'a' },
			{ "frontend", 1, 0, 'f' },
			{ "timeout", 1, 0, 't' },
			{ "verbose", 0, 0, 'v' },
			{ "min-freq", 1, 0, 'm' },
			{ "max-freq", 1, 0, 'M' },
			{ "step-freq", 1, 0, 's' },

		};

		char *args = "ha:f:t:vm:M:s:";

		int c = getopt_long(argc, argv, args, long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage(argv[0]);
				return 1;

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
			case 't':
				if (sscanf(optarg, "%u", &tuning_timeout) != 1) {
					printf("Invalid timeout \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				if (!tuning_timeout) {
					printf("Invalid tuning timeout, must be > 0\n");
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'v':
				verbose = 1;
				break;
			case 'm':
				if (sscanf(optarg, "%u", &freq_start) != 1) {
					printf("Invalid start frequency \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				freq_start *= 1000; // Switch to kHz
				break;
			case 'M':
				if (sscanf(optarg, "%u", &freq_end) != 1) {
					printf("Invalid end frequency \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				freq_end *= 1000; // Switch to kHz
				break;
			case 's':
				if (sscanf(optarg, "%u", &freq_step) != 1) {
					printf("Invalid frequency steps \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				freq_step *= 1000; // Switch to kHz
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

	unsigned int lnb_freq_start, lnb_freq_end;
	if (lnb_get_limits(lnb_type_univeral, &lnb_freq_start, &lnb_freq_end)) {
		printf("Error while getting LNB limits\n");
		goto err;
	}

	if (freq_start < lnb_freq_start)
		freq_start = lnb_freq_start;

	if (!freq_end || freq_end > lnb_freq_end)
		freq_end = lnb_freq_end;

	if (freq_start > freq_end) {
		printf("Invalid frequency range : start %u Mhz, end %u Mhz\n", freq_start, freq_end);
		goto err;
	}

	if (fe_info.frequency_stepsize > freq_step)
		freq_step = fe_info.frequency_stepsize;

	if (!freq_step) {
		printf("Frontend returned frequency step size < 1Mhz, defaulting to 1Mhz\n");
		freq_step = 1000;
	}

	frontend_print_info(&fe_info);

	scan(frontend_fd, tuning_timeout, freq_start, freq_end, freq_step);


	frontend_close(frontend_fd);

	return 0;

err:
	frontend_close(frontend_fd);
	return 1;

}



int fh_debug(const char *format, ...) {
	if (!verbose)
		return 0;

	int ret = 0;

	va_list arg_list;
	va_start(arg_list, format);
	ret = vprintf(format, arg_list);
	va_end(arg_list);

	return ret;
}


int fh_progress(unsigned int cur, unsigned int max) {

	if (verbose)
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
