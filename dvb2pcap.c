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
#include <string.h>
#include <stdlib.h>


#include "frontend.h"
#include "lnb.h"
#include "config.h"


#define PID_FULL_TS "8192"
#define MPEG_TS_LEN 188

#ifndef DLT_MPEG_2_TS
#define DLT_MPEG_2_TS DLT_USER0
#endif

static int run = 0;

void print_usage(char *app) {
	
	printf("Usage : %s <options>\n"
		"\n"
		"Options are :\n"
		" -h, --help                                      Display this help and exit\n"
		" -A, --adapter=X                                 Adapter to use\n"
		" -F, --frontend=X                                Frontend to use\n"
		" -D, --demux=X                                   Demux to use\n"
		" -T, --timeout=X                                 Tuning timeout in seconds, default 5\n"
		" -f, --frequency=X                               Frequency to tune to in Hz\n"
		" -s, --symbol-rate=X                             Symbol rate in Sym/s\n"
		" -p, --polarity=[h,v]                            Polarity (DVB-S only, default: h)\n"
		" -m, --modulation=[auto,16,32,64,128,256]        QAM modulation to use (DVB-C and T only, default: 256)\n"
		" -b, --bandwidth=[auto,6,7,8]                    Bandwidth in mHz (DVB-T only, default: 8)\n"
		" -t, --transmission-mode=[auto,2,8]              Transmission mode (DVB-T only, default: 8)\n"
		" -c, --code-rate=[auto,none,1_2,2_3,3_4,5_6,7_8] Code rate (DVB-T only, default: auto)\n"
		" -g, --guard-interval=[auto,4,8,16,32]           Guard interval 1_X (DVB-T only, default: auto)\n"
		" -o, --output=X                                  Output file (default: dvb.cap)\n"
		" -P, --pid=X<,Y,.>                               Capture specific PIDs (default: all)\n"
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
	fe_modulation_t modulation = QAM_256;
	fe_bandwidth_t bandwidth = BANDWIDTH_8_MHZ;
	fe_transmit_mode_t transmit_mode = TRANSMISSION_MODE_8K;
	fe_code_rate_t code_rate = FEC_AUTO;
	fe_guard_interval_t guard_interval = GUARD_INTERVAL_AUTO;

	unsigned long int pkt_count = 0;

	char polarity = 'h';
	char *output = "dvb.cap";

	unsigned int verbose = 0;


	char *pids = PID_FULL_TS;

	while (1) {
		static struct option long_options[] = {
			{ "help", 0, 0, 'h' },
			{ "adapter", 1, 0, 'A' },
			{ "frontend", 1, 0, 'F' },
			{ "demux", 1, 0, 'D' },
			{ "timeout", 1, 0, 'T' },
			{ "frequency", 1, 0, 'f' },
			{ "symbol-rate", 1, 0, 's' },
			{ "polarity", 1, 0, 'p' },
			{ "modulation", 1, 0, 'm' },
			{ "bandwidth", 1, 0, 'b' },
			{ "transmission-mode", 1, 0, 't' },
			{ "code-rate", 1, 0, 'c' },
			{ "guard-interval", 1, 0, 'g' },
			{ "output", 1, 0, 'o' },
			{ "pid", 1, 0, 'P' },
		};

		char *args = "hA:F:D:T:f:s:p:m:b:t:c:g:o:P:";

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
			case 'T':
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
			case 'm':
				if (!strcmp("auto", optarg)) {
					modulation = QAM_AUTO;
				} else if (!strcmp("16", optarg)) {
					modulation = QAM_16;
				} else if (!strcmp("32", optarg)) {
					modulation = QAM_32;
				} else if (!strcmp("64", optarg)) {
					modulation = QAM_64;
				} else if (!strcmp("128", optarg)) {
					modulation = QAM_128;
				} else if (!strcmp("256", optarg)) {
					modulation = QAM_256;
				} else {
					printf("Invalid modulation \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'b':
				if (!strcmp("auto", optarg)) {
					bandwidth = BANDWIDTH_AUTO;
				} else if (!strcmp("6", optarg)) {
					bandwidth = BANDWIDTH_6_MHZ;
				} else if (!strcmp("7", optarg)) {
					bandwidth = BANDWIDTH_7_MHZ;
				} else if (!strcmp("8", optarg)) {
					bandwidth = BANDWIDTH_8_MHZ;
				} else {
					printf("Invalid bandwidth \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 't':
				if (!strcmp("auto", optarg)) {
					transmit_mode = TRANSMISSION_MODE_AUTO;
				} else if (!strcmp("2", optarg)) {
					transmit_mode = TRANSMISSION_MODE_2K;
				} else if (!strcmp("8", optarg)) {
					transmit_mode = TRANSMISSION_MODE_8K;
				} else {
					printf("Invalid transmission mode \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'c':
				if (!strcmp("auto", optarg)) {
					code_rate = FEC_AUTO;
				} else if (!strcmp("none", optarg)) {
					code_rate = FEC_NONE;
				} else if (!strcmp("1_2", optarg)) {
					code_rate = FEC_1_2;
				} else if (!strcmp("2_3", optarg)) {
					code_rate = FEC_2_3;
				} else if (!strcmp("3_4", optarg)) {
					code_rate = FEC_3_4;
				} else if (!strcmp("5_6", optarg)) {
					code_rate = FEC_5_6;
				} else if (!strcmp("7_8", optarg)) {
					code_rate = FEC_7_8;
				} else {
					printf("Invalid code rate \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'g':
				if (!strcmp("auto", optarg)) {
					guard_interval = GUARD_INTERVAL_AUTO;
				} else if (!strcmp("4", optarg)) {
					guard_interval = GUARD_INTERVAL_1_4;
				} else if (!strcmp("8", optarg)) {
					guard_interval = GUARD_INTERVAL_1_8;
				} else if (!strcmp("16", optarg)) {
					guard_interval = GUARD_INTERVAL_1_16;
				} else if (!strcmp("32", optarg)) {
					guard_interval = GUARD_INTERVAL_1_32;
				} else {
					printf("Invalid guard time \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'o':
				output = optarg;
				break;
			case 'P':
				pids = optarg;
				break;

			default:
				print_usage(argv[0]);
				return 1;
		}
	}

	// Open the frontend

	char frontend_str[NAME_MAX];
	snprintf(frontend_str, NAME_MAX - 1, "/dev/dvb/adapter%u/frontend%u", adapter, frontend);

	struct dvb_frontend_info fe_info;

	int frontend_fd = frontend_open(frontend_str, &fe_info);
	if (frontend_fd == -1)
		return 1;

	frontend_print_info(&fe_info);

	switch (fe_info.type) {
		case FE_QPSK: {
			printf("Tuning to %u MHz, %u MSym/s, %c Polarity ...\n", frequency / 1000, symbol_rate / 1000, polarity);
			unsigned int ifreq = 0, hiband = 0;
			if (lnb_get_parameters(lnb_type_univeral, frequency, &ifreq, &hiband)) {	
				printf("Error while getting LNB parameters");
				return 1;
			}

			if (frontend_set_voltage(frontend_fd, (polarity == 'h' ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13)))
				return -1;

			if (frontend_set_tone(frontend_fd, (hiband ? SEC_TONE_ON : SEC_TONE_OFF)))
				return -1;

			if (frontend_tune_dvb_s(frontend_fd, ifreq, symbol_rate))
				return -1;
			break;
		}

		case FE_QAM: {
			printf("Tuning to %u MHz, %u MSym/s, %s ...\n", frequency / 1000, symbol_rate / 1000, (modulation == QAM_64 ? "QAM 64" : "QAM 256"));
			if (frontend_tune_dvb_c(frontend_fd, frequency, symbol_rate, modulation))
				return -1;
			break;

		}
		
		case FE_OFDM: {
			// Improve this message
			printf("Tuning to %u MHz ...\n", frequency / 1000);
			if (frontend_tune_dvb_t(frontend_fd, frequency, modulation, bandwidth, transmit_mode, code_rate, guard_interval))
				return -1;
			break;
		}


		default:
			printf("Unhandled frontend type\n");
			return -1;

	}

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



	// Setup the demux

	// Parse the PIDS
	int demux_fd_count = 0;
	int *demux_fd = NULL;
	char *my_pids = strdup(pids);
	char *str, *token, *saveptr = NULL;
	
	for (str = my_pids; ; str = NULL) {

		demux_fd = realloc(demux_fd, sizeof(int) * (demux_fd_count + 1));
		if (!demux_fd) {
			perror("Not enough memory");
			return 1;
		}


		demux_fd[demux_fd_count] = open(demux_str, O_RDWR);
		if (demux_fd[demux_fd_count] == -1) {
			perror("Error while opening the demux");
			return 1;
		}
		
		token = strtok_r(str, ",", &saveptr);
		if (!token)
			break;
		uint16_t pid;
		if (sscanf(token, "%hu", &pid) != 1) {
			printf("Unparseable PID : \"%s\"\n", token);
			return 1;
		}

		struct dmx_pes_filter_params filter = {0};
		filter.pid = pid;
		filter.input = DMX_IN_FRONTEND;
		filter.output = DMX_OUT_TS_TAP;
		filter.pes_type = DMX_PES_OTHER;
		filter.flags = DMX_IMMEDIATE_START;

		if (ioctl(demux_fd[demux_fd_count],  DMX_SET_PES_FILTER, &filter) != 0) {
			perror("Error while setting demux filter");
			return 1;
		}
	}

	free(my_pids);


	// Open the DVR
	
	char dvr_str[NAME_MAX];
	snprintf(dvr_str, NAME_MAX - 1, "/dev/dvb/adapter%u/dvr0", adapter);

	int dvr_fd = open(dvr_str, O_RDONLY);
	if (dvr_fd == -1) {
		perror("Error while opening the dvr device");
		return 1;
	}


	// Open pcap
	if (DLT_MPEG_2_TS == DLT_USER0)
		printf("Warning, your libpcap doesn't support DLT_MPEG_2_TS. Saving as DLT_USER0 instead.\n");
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

		pkt_count++;

		if (!(pkt_count % 1000)) {
			printf("\rGot %lu", pkt_count);
			fflush(stdout);
		}
	}


	pcap_dump_close(pcap_dumper);
	pcap_close(pcap);
	printf("\rDumped %lu packets\n", pkt_count);

	close(dvr_fd);
	int i;
	for (i = 0; i < demux_fd_count; i++)
		close(demux_fd[i]);
	free(demux_fd);
	close(frontend_fd);

}

