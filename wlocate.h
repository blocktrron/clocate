
#ifndef __WLOCATE_H
#define __WLOCATE_H

#include <time.h>

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

struct geolocation_result {
	double latitude;
	double longitude;
	double accuracy;

	time_t timestamp;
};

struct curl_output {
	char *outbuf;
	size_t len;
};

int geolocation_request(struct geolocation_result *result, struct curl_output *output,
			char *url, char *post_data);

int build_request_url(char **output, char *format_str, char *api_key);

int perform_locate(struct scan_results *results, struct geolocation_result *geolocation,
		   char *request_url);

#endif
