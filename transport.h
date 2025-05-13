#ifndef __CLOCATE_TRANSPORT_H
#define __CLOCATE_TRANSPORT_H

#include <stdbool.h>

struct transport_result {
	char *outbuf;
	size_t len;
};

int transport_get_file(struct transport_result *output, char *url, char *post_data, bool debug_output);

#endif
