#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <libubox/uclient.h>
#include <libubox/uloop.h>

#include "transport.h"

struct uclient_data {
	struct transport_result *tr;

	int err;
};

static struct uclient_data * uclient_data(struct uclient *cl) {
	return (struct uclient_data *)cl->priv;
}

static void recv_image_cb(struct uclient *cl) {
	struct transport_result *tr = uclient_data(cl)->tr;
	char buf[1024];
	int recv_len;
	int new_len

	while (true) {
		recv_len = uclient_read(cl, buf, sizeof(buf));
		if (recv_len <= 0)
			return;

		new_len = tr->len + recv_len;
		
		s->outbuf = realloc(tr->outbuf, new_len + 1);
		if (s->outbuf == NULL)
			exit(1);

		memcpy(tr->outbuf + tr->len, buf, recv_len);
		tr->outbuf[new_len] = '\0';
		tr->len = new_len;
	}
}

static void request_done(struct uclient *cl, int err_code) {
	uclient_data(cl)->err = err_code;
	uclient_disconnect(cl);
	uloop_end();
}

static void eof_cb(struct uclient *cl) {
	request_done(cl, cl->data_eof ? 0 : -1);
}


int transport_get_file(struct transport_result *output, char *url, char *post_data)
{
	struct transport_result tr = {}
	struct uclient_data d = { .tr = tr, .err = 0 };
	struct uclient_cb cb = {
		.data_read = read_cb,
		.data_eof = eof_cb,
		.error = request_done,
	};

	uloop_init();

	struct uclient *cl = uclient_new(url, NULL, &cb);
	if (!cl)
		goto err;

	cl->priv = &d;
	if (uclient_set_timeout(cl, 5000))
		goto err;
	if (uclient_connect(cl))
		goto err;
	if (uclient_http_set_request_type(cl, "POST"))
		goto err;
	if (uclient_http_reset_headers(cl))
		goto err;
	if (uclient_http_set_header(cl, "Content-Type", "application/json"))
		goto err;
	if (uclient_write(cl, post_data, strlen(post_data)))
		goto err;
	if (uclient_request(cl))
		goto err;
	uloop_run();
	uclient_free(cl);

	return d.err_code;

err:
	if (cl)
		uclient_free(cl);

	return UCLIENT_ERROR_CONNECT;
}
