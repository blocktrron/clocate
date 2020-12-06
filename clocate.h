
#ifndef __CLOCATE_H
#define __CLOCATE_H

#include <time.h>
#include <stdbool.h>

#define 	MAC2STR(a)   (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define 	MACSTR   "%02x:%02x:%02x:%02x:%02x:%02x"


#define MOZILLA_API_PATH	"https://location.services.mozilla.com/v1/geolocate?key=%s"
#define GOOGLE_API_PATH		"https://www.googleapis.com/geolocation/v1/geolocate?key=%s"

struct scan_result {
	unsigned char bssid[6];
	char ssid[33];
	int signal;
};

struct scan_results {
	int result_count;
	struct scan_result *results;
};

struct geolocation_provider {
	char *name;
	char *url;
	bool api_key;
	char *default_api_key;

	char * (*get_url)(struct geolocation_provider*, char *, char *);
};

struct geolocation_result {
	double latitude;
	double longitude;
	double accuracy;

	time_t timestamp;
};

struct if_results {
	char *buf;
	size_t count;
};

struct locator_config {
	struct if_results interfaces;

	struct geolocation_provider *provider;
	char *provider_url;
	char *provider_api_key;
};

int build_request_url(char **output, char *format_str, char *api_key);

int perform_locate(struct scan_results *results, struct geolocation_result *geolocation,
		   char *request_url);

struct geolocation_provider* get_geolocation_providers();

struct geolocation_provider* get_geolocation_provider(char *name);

#endif
