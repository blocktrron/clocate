#include <stdio.h>
#include <string.h>
#include <netlink/genl/genl.h>
#include <net/if.h>

#include "nl80211.h"
#include "clocate.h"


static void usage(char *path) {
	struct geolocation_provider *providers, *p;
	printf("Usage: %s provider [apikey]\n", path);

	providers = get_geolocation_providers();
	printf("Available providers:\n");
	for (p = providers; p->name; p++)
		printf(" %s\n", p->name);
	
}

int main(int argc, char *argv[]) {
	struct geolocation_result geolocation_result = {};
	struct geolocation_provider *geo_provider;
	struct scan_results results = {};
	struct if_results ifresults = {};
	char *request_url;
	char *interface;
	char *provider;
	char *apikey;
	int ret;

	get_wireless_interfaces(&ifresults);

	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	provider = argv[1];

	geo_provider = get_geolocation_provider(argv[1]);

	if (!geo_provider || argc < 3 && geo_provider->api_key && !geo_provider->default_api_key) {
		usage(argv[0]);
		exit(1);
	} else {
		apikey = argv[2];
	}

	request_url = geo_provider->get_url(geo_provider, NULL, apikey);
	if (!request_url)
		return 1;

	for (int i = 0; i < ifresults.count; i++)
		perform_scan(&results, &ifresults.buf[i * IF_NAMESIZE]);

	if (ret = perform_locate(&results, &geolocation_result, request_url))
		goto out;

	printf("%f, %f %f\n", geolocation_result.latitude, geolocation_result.longitude, geolocation_result.accuracy);

out:
	free(request_url);

	return 0;
}
