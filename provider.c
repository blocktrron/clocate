#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "nl80211.h"
#include "clocate.h"
#include "transport.h"

static int provider_build_submission_object(char **out, struct scan_results *results)
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

static int provider_json_response_parse(struct clocate_geolocation_result *result, char *response)
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

static int provider_build_request_url(char **output, char *format_str, char *api_key)
{
	size_t format_str_len = strlen(format_str);
	size_t api_key_len = strlen(api_key);
	size_t output_len = format_str_len + api_key_len - 2 + 1;

	*output = calloc(output_len, sizeof(char));
	if (!output)
		return -ENOMEM;

	snprintf(*output, output_len, format_str, api_key);
	return 0;
}

static int provider_perform_locate(struct scan_results *results, struct clocate_geolocation_result *geolocation,
		   char *request_url)
{
	char *request_obj_str;
	struct transport_result output = {};
	int ret = 0;

	provider_build_submission_object(&request_obj_str, results);

	if (ret = transport_get_file(&output, request_url, request_obj_str))
		goto out;

	provider_json_response_parse(geolocation, output.outbuf);

out:
	free(output.outbuf);
	free(request_obj_str);

	return ret;
}

char *provider_get_url(struct clocate_geolocation_provider *provider, char *url, char *api_key)
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

		provider_build_request_url(&out, provider->url, api_key);
	}

	return out;
}

struct clocate_geolocation_provider providers[] = {
	{.name = "beacondb", .url = BEACONDB_API_PATH, .get_url = provider_get_url, .api_key = false, .default_api_key = NULL},
	{.name = "mozilla", .url = MOZILLA_API_PATH, .get_url = provider_get_url, .api_key = true, .default_api_key = "test"},
	{.name = "google", .url = GOOGLE_API_PATH, .get_url = provider_get_url, .api_key = true, .default_api_key = NULL},
	{.name = "positon", .url = POSITON_API_PATH, .get_url = provider_get_url, .api_key = true, .default_api_key = "test"},
	{},
};

struct clocate_geolocation_provider* provider_get_geolocation_providers()
{
	return providers;
}

struct clocate_geolocation_provider* provider_get_geolocation_provider(char *name)
{
	struct clocate_geolocation_provider *pv;
	
	for (pv = &providers[0]; pv->name; pv++) {
		if (!strcmp(pv->name, name))
			return pv;
	}

	return NULL;
}

int provider_start_geolocation(struct clocate_config *configuration, struct clocate_geolocation_result *geo_result)
{
	struct scan_results net_results = {};
	char *request_url;
	int ret = 0;
	char *iface;

	request_url = configuration->provider->get_url(configuration->provider,
						       configuration->provider_url,
						       configuration->provider_api_key);
	if (!request_url)
		return -EINVAL;

	for (int i = 0; i < configuration->interfaces.count; i++) {
		iface = &configuration->interfaces.buf[i * IF_NAMESIZE];
		ret = perform_scan(&net_results, iface);
		if (ret)
			fprintf(stderr, "Scan failed on interface %s\n", iface);
	}

	if (net_results.result_count == 0) {
		fprintf(stderr, "No networks found\n");
		goto out;
	}

	ret = provider_perform_locate(&net_results, geo_result, request_url);
out:
	if (request_url)
		free(request_url);

	if (net_results.results)
		free(net_results.results);

	return ret;
}
