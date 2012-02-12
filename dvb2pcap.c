/*
 *  dvb2pcap: save a DVB stream into a pcap file
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


#include <stdio.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <pcap.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/dvb/dmx.h>
#include <signal.h>


#include "frontend.h"
#include "lnb.h"
#include "config.h"


#define PID_FULL_TS 0x2000
#define MPEG_TS_LEN 188

#define DLT_MPEG_2_TS DLT_USER0

static int run = 0;

void print_usage(char *app) {
	
	printf("Usage : %s <options>\n"
	
		,app);

}

void sighandler(int signal) {
	run = 0;
}

int main(int argc, char *argv[]) {

	printf("%s : Copyright " PACKAGE_BUGREPORT "\n\n", argv[0]);

	unsigned int adapter = 0;
	unsigned int frontend = 0;
	unsigned int demux = 0;
	unsigned int tuning_timeout = 3;
	unsigned int frequency = 0;
	unsigned int symbol_rate = 27500000;

	char polarity = 'h';
	char *output = NULL;

	unsigned int verbose = 0;

	uint16_t pid = PID_FULL_TS;

	while (1) {
		static struct option long_options[] = {
			{ "help", 0, 0, 'h' },
			{ "adapter", 1, 0, 'A' },
			{ "frontend", 1, 0, 'F' },
			{ "demux", 1, 0, 'D' },
			{ "timeout", 1, 0, 't' },
			{ "frequency", 1, 0, 'f' },
			{ "symbol-rate", 1, 0, 's' },
			{ "polarity", 1, 0, 'p' },
			{ "output", 1, 0, 'o' },
			{ "PID", 1, 0, 'P' },
		};

		char *args = "hA:F:D:t:f:s:p:o:P:";

		int c = getopt_long(argc, argv, args, long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage(argv[0]);
				return 1;
			case 'A':
				if (sscanf(optarg, "%u", &adapter) != 1) {
					printf("Invalid adapter \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'F':
				if (sscanf(optarg, "%u", &frontend) != 1) {
					printf("Invalid frontend \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'D':
				if (sscanf(optarg, "%u", &demux) != 1) {
					printf("Invalid demux \"%s\"\n", optarg);
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
			case 'f':
				if (sscanf(optarg, "%u", &frequency) != 1) {
					printf("Invalid frequency \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				frequency *= 1000; // Switch to kHz
				break;
			case 's':
				if (sscanf(optarg, "%u", &symbol_rate) != 1) {
					printf("Invalid symbol rate \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'p':
				if (optarg[0] == 'h' || optarg[0] == 'H') {
					polarity = 'h';
				} else if (optarg[0] = 'v' || optarg[0] == 'V') {
					polarity = 'v';
				} else {
					printf("Invalid polarity \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;

			case 'o':
				output = optarg;
				break;
			case 'P':
				if (sscanf(optarg, "%hu", &pid) != 1) {
					printf("Invalid PID \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;

			default:
				print_usage(argv[0]);
				return 1;
		}
	}

	if (!output) {
		printf("You must specify an output with -o\n");
		print_usage(argv[0]);
		return 1;
	}


	// Open the frontend

	char frontend_str[NAME_MAX];
	snprintf(frontend_str, NAME_MAX - 1, "/dev/dvb/adapter%u/frontend%u", adapter, frontend);

	struct dvb_frontend_info fe_info;

	int frontend_fd = frontend_open(frontend_str, &fe_info);
	if (frontend_fd == -1)
		return 1;

	printf("Tuning to %u Mhz, %u MSym/s, %c Polarity ...\n", frequency / 1000, symbol_rate / 1000, polarity);
	unsigned int ifreq = 0, hiband = 0;
	if (lnb_get_parameters(lnb_type_univeral, frequency, &ifreq, &hiband)) {	
		printf("Error while getting LNB parameters");
		return 1;
	}

	if (frontend_set_voltage(frontend_fd, (polarity == 'h' ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13)))
		return -1;

	if (frontend_set_tone(frontend_fd, (hiband ? SEC_TONE_ON : SEC_TONE_OFF)))
		return -1;

	if (frontend_tune(frontend_fd, ifreq, symbol_rate))
		return -1;

	fe_status_t status;
	if (frontend_get_status(frontend_fd, tuning_timeout, &status))
		return -1;

	if (!(status & FE_HAS_LOCK)) {
		printf("Lock not aquired :-/\n");
		return -1;
	}

	printf("Lock aquired\n");


	// Open the demux
	
	char demux_str[NAME_MAX];
	snprintf(demux_str, NAME_MAX - 1, "/dev/dvb/adapter%u/demux%u", adapter, frontend);


	int demux_fd = open(demux_str, O_RDWR);
	if (demux_fd == -1) {
		perror("Error while opening the demux");
		return 1;
	}

	// Setup the demux
	
	struct dmx_pes_filter_params filter = {0};
	filter.pid = pid;
	filter.input = DMX_IN_FRONTEND;
	filter.output = DMX_OUT_TS_TAP;
	filter.pes_type = DMX_PES_OTHER;
	filter.flags = DMX_IMMEDIATE_START;

	if (ioctl(demux_fd,  DMX_SET_PES_FILTER, &filter) != 0) {
		perror("Error while setting demux filter");
		return 1;
	}


	// Open the DVR
	
	char dvr_str[NAME_MAX];
	snprintf(dvr_str, NAME_MAX - 1, "/dev/dvb/adapter%u/dvr0", adapter);

	int dvr_fd = open(dvr_str, O_RDONLY);
	if (dvr_fd == -1) {
		perror("Error while opening the dvr device");
		return 1;
	}


	// Open pcap
	pcap_t *pcap = pcap_open_dead(DLT_MPEG_2_TS, MPEG_TS_LEN);

	if (!pcap) {
		perror("Error while opening pcap");
		return 1;
	}

	pcap_dumper_t *pcap_dumper = pcap_dump_open(pcap, output);
	if (!pcap_dumper) {
		pcap_perror(pcap, "Error while opening pcap dumper");
		return 1;
	}


	// Read packets and save them

	unsigned char buff[MPEG_TS_LEN];

	run = 1;
	
	// Install the signal handlers
	signal(SIGINT, sighandler);
	signal(SIGHUP, sighandler);

	printf("Dumping packets ...\n");

	while (run) {

		ssize_t len = 0, r  = 0;
		// Fetch a packet
		do {
			r = read(dvr_fd, buff + len, MPEG_TS_LEN - len);
			if (!run)
				break;
			if (r < 0) {
				if (errno = EOVERFLOW) {
					printf("Buffer overflow, your computer is too slow !!!\n");
					len = 0;
					r = 0;
					continue;
				}
			}

			len += r;

		} while (len < MPEG_TS_LEN && run);

		struct pcap_pkthdr phdr = {0};
		gettimeofday(&phdr.ts, NULL);
		phdr.caplen = MPEG_TS_LEN;
		phdr.len = MPEG_TS_LEN;

		// Save the packet
		pcap_dump((u_char*)pcap_dumper, &phdr, buff);
	}


	pcap_dump_close(pcap_dumper);
	pcap_close(pcap);
	printf("Pcap closed\n");

	close(dvr_fd);
	close(demux_fd);
	close(frontend_fd);

}

