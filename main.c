#include <stdio.h>
#include <string.h>
#include <netlink/genl/genl.h>
#include <net/if.h>

#include "nl80211.h"
#include "wlocate.h"


static void usage(char *p) {
	printf("Usage: %s provider [apikey]\n", p);
	printf("Available providers: mozilla google\n");
}

int main(int argc, char *argv[]) {
	struct geolocation_result geolocation_result = {};
	struct scan_results results = {};
	struct if_results ifresults = {};
	char *request_url;
	char *interface;
	char *provider;
	char *apikey;

	get_wireless_interfaces(&ifresults);

	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	provider = argv[1];

	if (argc < 3 && !strcmp("mozilla", provider)) {
		apikey = NULL;
	} else if (argc < 3) {
		usage(argv[0]);
		exit(1);
	} else {
		apikey = argv[2];
	}

	for (int i = 0; i < ifresults.count; i++)
		perform_scan(&results, &ifresults.buf[i * IF_NAMESIZE]);

	if (!strcmp("mozilla", provider)) {
		if (build_request_url(&request_url, MOZILLA_API_PATH, apikey))
			return 1;
	} else {
		if (build_request_url(&request_url, GOOGLE_API_PATH, apikey))
			return 1;
	}

	perform_locate(&results, &geolocation_result, request_url);

	printf("%f, %f %f\n", geolocation_result.latitude, geolocation_result.longitude, geolocation_result.accuracy);

	free(request_url);

	return 0;
}
