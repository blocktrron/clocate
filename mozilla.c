#include <json-c/json.h>
#include <string.h>
#include <curl/curl.h>
#include <stdio.h>

#include "nl80211.h"
#include "mozilla.h"
#include "wlocate.h"

char *outbuf;
size_t len = 0;

size_t writefunc(void *ptr, size_t size, size_t nmemb, void *s)
{
	size_t new_len = len + size*nmemb;
	outbuf = realloc(outbuf, new_len+1);
	if (outbuf == NULL)
		exit(1);

	memcpy(outbuf+len, ptr, size*nmemb);
	outbuf[new_len] = '\0';
	len = new_len;

	return size*nmemb;
}

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
	json_object_object_get_ex(location_obj, "accuracy", &jobj);
	result->accuracy = json_object_get_double(jobj);

	json_object_put(root_obj);
}

static int geolocate(struct geolocation_result *result, char *post_data)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist *list = NULL;
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */ 
	curl = curl_easy_init();
	if (curl) {
		/* First set the URL that is about to receive our POST. This URL can
		just as well be a https:// URL if that is what should receive the
		data. */ 
		curl_easy_setopt(curl, CURLOPT_URL, "https://location.services.mozilla.com/v1/geolocate?key=test");
		/* Now specify the POST data */ 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

		list = curl_slist_append(list, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}

int locate_mozilla(struct scan_results *results, struct geolocation_result *geolocation)
{
	char *request_obj_str;

	build_submission_object(&request_obj_str, results);

	geolocate(geolocation, request_obj_str);

	json_response_parse(geolocation, outbuf);

	free(outbuf);
	free(request_obj_str);

	return 0;

}