
#ifndef __WLOCATE_H
#define __WLOCATE_H

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

#endif
