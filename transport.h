#ifndef __CLOCATE_TRANSPORT_H
#define __CLOCATE_TRANSPORT_H

struct transport_result {
	char *outbuf;
	size_t len;
};

int get_file(struct transport_result *output, char *url, char *post_data);

#endif
