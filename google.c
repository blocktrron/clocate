#include <json-c/json.h>
#include <string.h>
#include <curl/curl.h>
#include <stdio.h>

#include "nl80211.h"
#include "google.h"
#include "wlocate.h"

#define API_FORMAT_STR	"https://www.googleapis.com/geolocation/v1/geolocate?key=%s"


static int build_submission_object(char **out, struct scan_results *results)
{
	struct json_object *root_obj, *ap_list;
	size_t bufsize;
	char *c;

	root_obj = json_object_new_object();
	ap_list = json_object_new_array();

	for (int i = 0; i < results->result_count; i++) {
		struct scan_result *result = &results->results[i];
		struct json_object *ap_obj = json_object_new_object();
		char mac_str[3*6];

		snprintf(mac_str, 3*6, MACSTR, MAC2STR(result->bssid));
		json_object_object_add(ap_obj, "macAddress", json_object_new_string(mac_str));
		json_object_object_add(ap_obj, "signalStrength", json_object_new_int(result->signal));
		json_object_array_add(ap_list, ap_obj);
	}

	json_object_object_add(root_obj, "considerIp", json_object_new_boolean(0));
	json_object_object_add(root_obj, "wifiAccessPoints", ap_list);
	c = json_object_to_json_string(root_obj);

	bufsize = strlen(c) + 1;
	*out = calloc(bufsize, sizeof(char));
	snprintf(*out, bufsize, "%s", c);

	json_object_put(root_obj);

	return 0;
}

static int json_response_parse(struct geolocation_result *result, char *response)
{
	struct json_object *root_obj, *location_obj, *jobj;

	root_obj = json_tokener_parse(response);

	json_object_object_get_ex(root_obj, "location", &location_obj);
	json_object_object_get_ex(location_obj, "lat", &jobj);
	result->latitude = json_object_get_double(jobj);
	json_object_object_get_ex(location_obj, "lng", &jobj);
	result->longitude = json_object_get_double(jobj);
	json_object_object_get_ex(root_obj, "accuracy", &jobj);
	result->accuracy = json_object_get_double(jobj);

	json_object_put(root_obj);
}

int locate_google(struct scan_results *results, struct geolocation_result *geolocation, char *api_key)
{
	char *request_obj_str;
	struct curl_output output = {};
	char *request_url;

	if (build_request_url(&request_url, API_FORMAT_STR, api_key))
		return 1;

	build_submission_object(&request_obj_str, results);

	geolocation_request(geolocation, &output, request_url, request_obj_str);

	json_response_parse(geolocation, output.outbuf);

	free(request_url);
	free(output.outbuf);
	free(request_obj_str);

	return 0;

}