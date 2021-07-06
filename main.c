#include <stdio.h>
#include <string.h>
#include <netlink/genl/genl.h>
#include <net/if.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <json-c/json.h>

#include "nl80211.h"
#include "clocate.h"


static void usage(char *path) {
	struct geolocation_provider *providers, *p;
	printf("Usage: %s [-h] [-j] [-p provider] [-k apikey] [-i interface]\n", path);

	providers = provider_get_geolocation_providers();
	printf("Available providers:\n");
	for (p = providers; p->name; p++)
		printf(" %s\n", p->name);
	
}

static void print_json(struct geolocation_result *result) {
	struct json_object *root_obj, *ap_list;
	size_t bufsize;
	const char *c;

	root_obj = json_object_new_object();

	json_object_object_add(root_obj, "lat", json_object_new_double(result->latitude));
	json_object_object_add(root_obj, "lon", json_object_new_double(result->longitude));
	json_object_object_add(root_obj, "accuracy", json_object_new_double(result->accuracy));

	c = json_object_to_json_string(root_obj);
	printf("%s\n", c);
	json_object_put(root_obj);
}

int main(int argc, char *argv[]) {
	struct geolocation_result geolocation_result = {};
	struct locator_config configuration = { .json_output = false };
	int ret;
	char c;

	while ((c = getopt (argc, argv, "hji:k:p:")) != -1) {
		switch(c) {
			case 'h':
				usage(argv[0]);
				exit(0);
			case 'j':
				configuration.json_output = true;
				break;
			case 'i':
				if(strlen(optarg) > IF_NAMESIZE - 1) {
					usage(argv[0]);
					exit(1);
				}

				configuration.interfaces.count = 1;
				configuration.interfaces.buf = malloc(IF_NAMESIZE * sizeof(char));
				snprintf(configuration.interfaces.buf, IF_NAMESIZE, "%s", optarg);
				break;
			case 'p':
				configuration.provider = provider_get_geolocation_provider(optarg);
				break;
			case 'k':
				configuration.provider_api_key = optarg;
				break;
		}
	}

	if (!configuration.interfaces.buf)
		get_wireless_interfaces(&configuration.interfaces);

	if (configuration.interfaces.count == 0) {
		fprintf(stderr, "No wireless interfaces available\n");
		goto out;
	}

	if (!configuration.provider)
		configuration.provider = provider_get_geolocation_provider("mozilla");

	if (configuration.provider->api_key && !configuration.provider->default_api_key &&
	    !configuration.provider_api_key) {
		usage(argv[0]);
		exit(1);
	}

	if (ret = provider_start_geolocation(&configuration, &geolocation_result))
		goto out;

	if (configuration.json_output)
		print_json(&geolocation_result);
	else
		printf("%f, %f %f\n", geolocation_result.latitude, geolocation_result.longitude, geolocation_result.accuracy);

out:
	if (configuration.interfaces.buf)
		free(configuration.interfaces.buf);
	return ret;
}
