
#include <stdio.h>
#include <getopt.h>
#include <math.h>

#define USALS_CLARKE_BELT_RADIUS	42164
#define USALS_EARTH_RADIUS		6348
#define PI				3.14159265


double usals_deg_to_rad(double deg) {
	return deg * PI / 180.0;
}

double usals_rad_to_deg(double rad) {
	return rad * 180.0 / PI;
}

double usals(double lon, double lat, double satpos) {

	// Longitute difference with the given satpos
	double dlon_rad	= usals_deg_to_rad(satpos - lon);

	// Latitude
	double lat_rad = usals_deg_to_rad(lat);


	// Formula :
	// Rotor angle = ArcTan { Rc.Sin(DLong) / [Rc.Cos(DLong) − Re.Cos(Lat)] }
	// Where :
	// Rc = Clarke belt radius
	// Re = Earth radius
	// http://www.macfh.co.uk/JavaJive/AudioVisualTV/SatelliteTV/SatelliteAnalysisRotor.html

	double dlon_rotor_rad = atan(  (USALS_CLARKE_BELT_RADIUS * sin(dlon_rad)) / ((USALS_CLARKE_BELT_RADIUS * cos(dlon_rad)) - (USALS_EARTH_RADIUS * cos (lat_rad))));

		return usals_rad_to_deg(dlon_rotor_rad);

}

int main(int argc, char *argv[]) {

	double lon = 0.0, lat = 0.0, satpos = 0.0;

	while (1) {
		static struct option long_options[] = {
			{ "latitute", 1, 0, 'y' },
			{ "longitude", 1, 0, 'x' },
			{ "satellite", 1, 0, 's' },
		};

		char *args = "x:y:s:";

		int c = getopt_long(argc, argv, args, long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'x':
				if (sscanf(optarg, "%lf", &lon) != 1) {
					printf("Invalid longitude\n");
					return 1;
				}
				break;
			case 'y':
				if (sscanf(optarg, "%lf", &lat) != 1) {
					printf("Invalid latitude\n");
					return 1;
				}
				break;
			case 's':
				if (sscanf(optarg, "%lf", &satpos) != 1) {
					printf("Invalid satellite position\n");
					return 1;
				}
				break;
			default:
				printf("Invalid argument\n");
				return 1;
		}
	}


	printf("Rotor angle is %f\n", usals(lon, lat, satpos));


	return 0;

}
