
#include <stdio.h>
#include <string.h>
#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>

#define PID_FULL_TS 0x2000

void sighandler(int signum) {

	if (signum == SIGINT) {
		// Send SIGCONT to wake up sleep();
		raise(SIGCONT);
		printf("Signal received.\n");
		return;
	}

}

void print_usage(char *app) {

	printf("Usage : %s <options> command\n"
		"\n"
		"Options are :\n"
		" -a, --adapter=X        Adapter to use\n"
		" -f, --frontend=X       Frontend to use\n"
		" -v, --voltage=<13|18>  Select bus voltage\n"
		" -t, --timeout=X        Timeout when moving\n"
		"\n"
		"Commands are :\n"
		" limits_off : Disable the rotor soft limits\n"
		" go_<east|west> : Go to east/west until timeout or interrupted\n"
		" go_<east|west> step X : Go to east/west for X step(s)\n"
		" go_<east|west> timeout X : Go to east/west, enforcing the rotor timeout until timeout or interrupted\n"
		" stop : Stop the rotor\n"
		" limit_set_<east|west> : Set east/west limit\n"
		" goto_sat X : Go to stored satellite X\n"
		" store_sat X : Store current orientation as satellite X\n"
		" goto_x XX.X<e|w> : Go to orientation XX.X either east or west\n"
		"\n"
		, app);


}

int main(int argc, char *argv[]) {


	// Parse command line
	unsigned int adapter = 0;
	unsigned int frontend = 0;
	unsigned int timeout = 180;

	enum fe_sec_voltage voltage = SEC_VOLTAGE_18;

	while (1) {

		static struct option long_options[] = {
			{ "adapter", 1, 0, 'a' },
			{ "frontend", 1, 0, 'f' },
			{ "timeout", 1, 0, 't' },
			{ "voltage", 1, 0, 'v' },

		};

		char *args = "a:f:t:v:";

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
			case 't':
				if (sscanf(optarg, "%u", &timeout) != 1) {
					printf("Invalid timeout \"%s\"\n", optarg);
					print_usage(argv[0]);
					return 1;
				}
				break;
			case 'v':
				if (!strcmp(optarg, "13")) {
					voltage = SEC_VOLTAGE_13;
				} else if (!strcmp(optarg, "18")) {
					voltage = SEC_VOLTAGE_18;
				} else {
					printf("Invalid voltage, must be either 13 or 18");
					print_usage(argv[0]);
					return 1;
				}
				break;
			default:
				print_usage(argv[0]);
				return 1;
		}

	}

	if (optind >= argc) {
		printf("You must specifiy an action\n");
		print_usage(argv[0]);
		return 1;
	}
	char *action = argv[optind];
	struct dvb_diseqc_master_cmd *cmd = NULL;
	unsigned int cmd_timeout = 0; // Default no timeout
	int do_stop = 0;

	static struct dvb_diseqc_master_cmd stop_cmd = { { 0xe0, 0x31, 0x60, 0x00, 0x00, 0x00 }, 3 };
	if (!strcmp(action, "stop")) {
		cmd = &stop_cmd;
	} else if (!strcmp(action, "limits_off")) {
		static struct dvb_diseqc_master_cmd limits_off_cmd = { { 0xe0, 0x31, 0x63, 0x00, 0x00, 0x00 }, 3 };
		cmd = &limits_off_cmd;
	} else if (!strcmp(action, "limit_set_east")) {
		static struct dvb_diseqc_master_cmd limit_set_east_cmd = { { 0xe0, 0x31, 0x66, 0x00, 0x00, 0x00 }, 3 };
		cmd = &limit_set_east_cmd;
	} else if (!strcmp(action, "limit_set_west")) {
		static struct dvb_diseqc_master_cmd limit_set_west_cmd = { { 0xe0, 0x31, 0x67, 0x00, 0x00, 0x00 }, 3 };
		cmd = &limit_set_west_cmd;
	} else if (!strcmp(action, "go_east") || !strcmp(action, "go_west")) {
		if (!strcmp(action, "go_east")) {
	 		static struct dvb_diseqc_master_cmd go_east_cmd = { { 0xe0, 0x31, 0x68, 0x00, 0x00, 0x00 }, 4 };
			cmd = &go_east_cmd;
		} else {
		 	static struct dvb_diseqc_master_cmd go_west_cmd = { { 0xe0, 0x31, 0x69, 0x00, 0x00, 0x00 }, 4 };
			cmd = &go_west_cmd;
		}

		cmd_timeout = timeout;
		do_stop = 1; // Force the rotor to stop

		if (optind + 2 < argc) {
			if (!strcmp(argv[optind + 1], "timeout")) {
				unsigned char timeout;
				if (sscanf(argv[optind + 2], "%hhu", &timeout) != 1 || (!timeout && timeout > 0x7F)) {
					printf("Invalid timeout \"%s\"\n", argv[optind + 2]);
					print_usage(argv[0]);
					return 1;
				}
				cmd->msg[3] = timeout;
			} else if (!strcmp(argv[optind + 1], "step")) {
				unsigned char steps;
				if (sscanf(argv[optind + 2], "%hhu", &steps) != 1 || (!steps && steps > 0x7F)) {
					printf("Invalid number of steps provided \"%s\"\n", argv[optind + 2]);
					print_usage(argv[0]);
					return 1;
				}
				cmd->msg[3] = steps | 0x80;
				cmd_timeout = steps * 30;
			} else {
				printf("Invalid argument \"%s\"\n", argv[optind + 1]);
				print_usage(argv[0]);
				return 1;
			}

		} else if (optind + 1 < argc) {
			printf("Incomplete command \"%s %s\"\n", action, argv[optind + 1]);
			print_usage(argv[0]);
			return 1;
		}

	} else if (!strcmp(action, "store_sat")) {
		++optind;
		if (optind >= argc) {
			printf("You must specify a position number where to store the current sat\n");
			print_usage(argv[0]);
			return 1;
		}
		unsigned char pos;
		if (sscanf(argv[optind], "%hhu", &pos) != 1) {
			printf("Invalid position \"%s\"\n", argv[optind]);
			print_usage(argv[0]);
			return 1;
		}
		static struct dvb_diseqc_master_cmd store_sat_cmd = { { 0xe0, 0x31, 0x6A, 0x00, 0x00, 0x00 }, 4 };
		store_sat_cmd.msg[3] = pos;
		cmd = &store_sat_cmd;
	} else if (!strcmp(action, "goto_sat")) {
		++optind;
		if (optind >= argc) {
			printf("You must specify a position number where to go to\n");
			print_usage(argv[0]);
			return 1;
		}
		unsigned char pos;
		if (sscanf(argv[optind], "%hhu", &pos) != 1) {
			printf("Invalid position \"%s\"\n", argv[optind]);
			print_usage(argv[0]);
			return 1;
		}
		static struct dvb_diseqc_master_cmd goto_sat_cmd = { { 0xe0, 0x31, 0x6B, 0x0, 0x00, 0x00 }, 4 };
		goto_sat_cmd.msg[3] = pos;
		cmd = &goto_sat_cmd;
		cmd_timeout = timeout;
	} else if (!strcmp(action, "goto_x")) {
		++optind;
		if (optind >= argc) {
			printf("You must specify the angle where to point\n");
			print_usage(argv[0]);
			return 1;
		}
		double angle;
		if (sscanf(argv[optind], "%lf", &angle) != 1) {
			printf("Invalid orientation provided \"%s\"\n", argv[optind]);
			print_usage(argv[0]);
			return 1;
		}
		char *dir = argv[optind] + strlen(argv[optind]) - 1;
		if (*dir != 'e' && *dir != 'w') {
			printf("Invalid direction \"%c\"\n", *dir);
			print_usage(argv[0]);
			return 1;
		}
		if (angle > 90.0) {
			printf("Angle out of range (0..90) : %f\n", angle);
			print_usage(argv[0]);
			return 1;
		}

		static struct dvb_diseqc_master_cmd goto_x_cmd = { { 0xe0, 0x31, 0x6E, 0x00, 0x00, 0x00 }, 5 };
		cmd = &goto_x_cmd;

	
		if (*dir == 'e') {
			// east
			cmd->msg[3] = 0xE0;
		}


		cmd->msg[3] += (unsigned char) (angle / 16.0);
		// Get the modulo 16
		cmd->msg[4] += ((unsigned char) (((angle / 16.0) - (int) (angle / 16.0)) * 16)) << 4;

		unsigned char decimal[10] = { 0x00, 0x2, 0x3, 0x5, 0x6, 0x8, 0xA, 0xB, 0xD, 0xE };
		angle -= (int) angle;
		cmd->msg[4] += decimal[(unsigned char) (angle * 10)];

		cmd_timeout = timeout;
	} else {
		printf("Invalid command provided\n");
		print_usage(argv[0]);
		return 1;
	}
	

	char frontend_str[NAME_MAX];
	snprintf(frontend_str, NAME_MAX - 1, "/dev/dvb/adapter%u/frontend%u", adapter, frontend);
	
	printf("Opening frontend %s\n", frontend_str);
	int frontend_fd = open(frontend_str, O_RDWR);

	if (frontend_fd == -1) {
		printf("Error while opening frontend : %s\n", strerror(errno));
		goto err;
	}

	if (ioctl(frontend_fd, FE_SET_TONE, SEC_TONE_OFF)) {
		printf("Error while turning tone off : %s\n", strerror(errno));
		goto err;
	}

	printf("Setting voltage to %uv\n", (voltage == SEC_VOLTAGE_13 ? 13 : 18));
	if (ioctl(frontend_fd, FE_SET_VOLTAGE, voltage)) {
		printf("Error while setting voltage : %s\n", strerror(errno));
		goto err;
	}

	printf("Waiting for the rotor to initialize ...\n");
	sleep(1);

	printf("Sending DiSEqC command : ");
	int i;
	for (i = 0; i < cmd->msg_len; i++)
		printf("0x%02X ", cmd->msg[i]);
	printf("\n");
	
		
	if (ioctl(frontend_fd, FE_DISEQC_SEND_MASTER_CMD, cmd)) {
		printf("Error while sending DiSEqC command : %s\n", strerror(errno));
		goto err;
	}
	

	if (cmd_timeout) {
		printf("Waiting for the command to execute (%u secs) ...\n", cmd_timeout);
		if (do_stop) {
			// ignore SIGINT so sleep can be interrupted safely
			signal(SIGINT, sighandler);
		}
		sleep(cmd_timeout);
	}

	if (do_stop) {
		printf("Stopping rotor\n");
		if (ioctl(frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &stop_cmd)) {
			printf("Error while sending DiSEqC command : %s\n", strerror(errno));
			goto err;
		}
	}

	close(frontend_fd);

	return 0;

err:
	if (frontend_fd != -1)
		close(frontend_fd);

	return 1;
}

