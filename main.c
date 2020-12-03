#include <stdio.h>
#include <string.h>
#include <netlink/genl/genl.h>

#include "nl80211.h"
#include "google.h"
#include "mozilla.h"
#include "wlocate.h"


static void usage(char *p) {
	printf("Usage: %s interface provider [apikey]\n", p);
	printf("Available providers: mozilla google\n");
}

int main(int argc, char *argv[]) {
	struct geolocation_result geolocation_result = {};
	struct scan_results results = {};
	char *interface;
	char *provider;
	char *apikey;

	if (argc < 3) {
		usage(argv[0]);
		exit(1);
	}

	interface = argv[1];
	provider = argv[2];

	if (argc < 4 && !strcmp("mozilla", provider)) {
		apikey = NULL;
	} else if (argc < 4) {
		usage(argv[0]);
		exit(1);
	} else {
		apikey = argv[3];
	}

	perform_scan(&results, interface);

	if (!strcmp("mozilla", provider))
		locate_mozilla(&results, &geolocation_result, apikey);
	else
		locate_google(&results, &geolocation_result, apikey);

	printf("%f, %f %f\n", geolocation_result.latitude, geolocation_result.longitude, geolocation_result.accuracy);

	return 0;
}
