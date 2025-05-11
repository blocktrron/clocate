
#ifndef __CLOCATE_H
#define __CLOCATE_H

#include <time.h>
#include <stdbool.h>

#define 	MAC2STR(a)   (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define 	MACSTR   "%02x:%02x:%02x:%02x:%02x:%02x"


#define BEACONDB_API_PATH	"https://api.beacondb.net/v1/geolocate"
#define MOZILLA_API_PATH	"https://location.services.mozilla.com/v1/geolocate?key=%s"
#define GOOGLE_API_PATH		"https://www.googleapis.com/geolocation/v1/geolocate?key=%s"
#define POSITON_API_PATH	"https://api.positon.xyz/v1/geolocate?key=%s"

struct clocate_geolocation_provider {
	char *name;
	char *url;
	bool api_key;
	char *default_api_key;

	char * (*get_url)(struct clocate_geolocation_provider*, char *, char *);
};

struct clocate_geolocation_result {
	double latitude;
	double longitude;
	double accuracy;

	time_t timestamp;
};

struct clocate_interfaces {
	char *buf;
	size_t count;
};

struct clocate_config {
	struct clocate_interfaces interfaces;

	struct clocate_geolocation_provider *provider;
	char *provider_url;
	char *provider_api_key;

	bool json_output;
};

struct clocate_geolocation_provider* provider_get_geolocation_providers();

struct clocate_geolocation_provider* provider_get_geolocation_provider(char *name);

int provider_start_geolocation(struct clocate_config *configuration, struct clocate_geolocation_result *geo_result);

#endif
