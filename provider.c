#include <string.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

#include "wlocate.h"

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

int geolocation_request(struct geolocation_result *result, struct curl_output *output,
			char *url, char *post_data)
{
	struct curl_slist *list = NULL;
	CURLcode res = 0;
	int ret = 0;
	CURL *curl;

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

	curl_easy_cleanup(curl);

out:
	if (res && res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
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