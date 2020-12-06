#include <stdio.h>
#include <string.h>
#include <netlink/genl/genl.h>
#include <net/if.h>
#include <errno.h>

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

static int start_geolocation(struct locator_config *configuration, struct geolocation_result *geo_result)
{
	struct scan_results net_results = {};
	char *request_url;
	int ret = 0;

	request_url = configuration->provider->get_url(configuration->provider,
						       configuration->provider_url,
						       configuration->provider_api_key);
	if (!request_url)
		return -EINVAL;

	for (int i = 0; i < configuration->interfaces.count; i++) {
		ret = perform_scan(&net_results, &configuration->interfaces.buf[i * IF_NAMESIZE]);
		if (ret)
			goto out;
	}

	ret = perform_locate(&net_results, geo_result, request_url);
out:
	if (request_url)
		free(request_url);

	if (net_results.results)
		free(net_results.results);

	return ret;
}

int main(int argc, char *argv[]) {
	struct geolocation_result geolocation_result = {};
	struct locator_config configuration = {};
	struct geolocation_provider *geo_provider;
	struct scan_results results = {};
	struct if_results ifresults = {};
	char *request_url;
	char *interface;
	char *provider;
	char *apikey;
	int ret;

	get_wireless_interfaces(&configuration.interfaces);

	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	provider = argv[1];

	configuration.provider = get_geolocation_provider(argv[1]);

	if (!configuration.provider ||
	    argc < 3 && configuration.provider->api_key && !configuration.provider->default_api_key) {
		usage(argv[0]);
		exit(1);
	} else {
		configuration.provider_api_key = argv[2];
	}

	if (ret = start_geolocation(&configuration, &geolocation_result))
		goto out;

	printf("%f, %f %f\n", geolocation_result.latitude, geolocation_result.longitude, geolocation_result.accuracy);

out:
	return ret;
}
