#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "clocate.h"

static size_t writefunc(void *ptr, size_t size, size_t nmemb, struct curl_output *s)
{
	size_t receiving_size = size * nmemb;
	size_t new_len = s->len + receiving_size;

	s->outbuf = realloc(s->outbuf, new_len + 1);
	if (s->outbuf == NULL)
		exit(1);

	memcpy(s->outbuf + s->len, ptr, receiving_size);
	s->outbuf[new_len] = '\0';
	s->len = new_len;

	return receiving_size;
}

static int build_submission_object(char **out, struct scan_results *results)
{
	struct json_object *root_obj, *ap_list;
	size_t bufsize;
	const char *c;

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

int geolocation_request(struct geolocation_result *result, struct curl_output *output,
			char *url, char *post_data)
{
	struct curl_slist *list = NULL;
	CURLcode res = 0;
	int ret = 0;
	CURL *curl;
	long http_code = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (!curl) {
		ret = -1;
		goto out;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

	list = curl_slist_append(list, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, output);

	res = curl_easy_perform(curl);
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

out:
	if (res && res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		ret = -EINVAL;
	} else if (http_code != 200)
		fprintf(stderr, "%s: %d\n", __func__, http_code);
		ret = -EINVAL;

	if (curl)
		curl_easy_cleanup(curl);
	curl_global_cleanup();
	return ret;
}

int build_request_url(char **output, char *format_str, char *api_key)
{
	size_t format_str_len = strlen(format_str);
	size_t api_key_len = strlen(api_key);
	size_t output_len = format_str_len + api_key_len - 2 + 1;

	*output = calloc(output_len, sizeof(char));
	if (!output)
		return 1;

	snprintf(*output, output_len, format_str, api_key);
	return 0;
}

int perform_locate(struct scan_results *results, struct geolocation_result *geolocation,
		   char *request_url)
{
	char *request_obj_str;
	struct curl_output output = {};
	int ret = 0;

	build_submission_object(&request_obj_str, results);

	if (ret = geolocation_request(geolocation, &output, request_url, request_obj_str))
		goto out;
		

	json_response_parse(geolocation, output.outbuf);

out:
	free(output.outbuf);
	free(request_obj_str);

	return ret;
}

char *provider_get_url(struct geolocation_provider *provider, char *url, char *api_key)
{
	char *out = NULL;

	if (url) {
		out = calloc(strlen(url) + 1, sizeof(char));
		memcpy(out, url, strlen(url) * sizeof(char));
	} else if (!provider->api_key) {
		if (!provider->url)
			return NULL;

		out = calloc(strlen(provider->url) + 1, sizeof(char));
		memcpy(out, provider->url, strlen(provider->url) * sizeof(char));
	} else {
		if (!api_key && !provider->default_api_key)
			return NULL;

		if (!api_key)
			api_key = provider->default_api_key;

		build_request_url(&out, provider->url, api_key);
	}

	return out;
}

struct geolocation_provider providers[] = {
	{.name = "mozilla", .url = MOZILLA_API_PATH, .get_url = provider_get_url, .api_key = true, .default_api_key = "test"},
	{.name = "google", .url = GOOGLE_API_PATH, .get_url = provider_get_url, .api_key = true, .default_api_key = NULL},
	{},
};

struct geolocation_provider* get_geolocation_providers()
{
	return providers;
}

struct geolocation_provider* get_geolocation_provider(char *name)
{
	struct geolocation_provider *pv;
	
	for (pv = &providers[0]; pv->name; pv++) {
		if (!strcmp(pv->name, name))
			return pv;
	}

	return NULL;
}
