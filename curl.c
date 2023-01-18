#include <string.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "log.h"
#include "transport.h"

static size_t curl_writefunc(void *ptr, size_t size, size_t nmemb, struct transport_result *s)
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

int transport_get_file(struct transport_result *output, char *url, char *post_data)
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
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, output);

	res = curl_easy_perform(curl);
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

out:
	if (res && res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		ret = -EINVAL;
	} else if (http_code != 200) {
		LOG_ERR_RET(http_code);
		ret = -EINVAL;
	}

	if (curl)
		curl_easy_cleanup(curl);

	if (list)
		curl_slist_free_all(list);

	curl_global_cleanup();
	return ret;
}
