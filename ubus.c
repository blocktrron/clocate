#include <libubox/blobmsg.h>
#include <libubox/uloop.h>
#include <libubox/utils.h>
#include <libubus.h>

#include "nl80211.h"
#include "clocate.h"

struct geolocation_result current_location = {};
static struct blob_buf b;

void clocate_ubus_save_location(struct geolocation_result *location)
{
	memcpy(&current_location, location, sizeof(current_location));
}

static int
clocate_ubus_get_location(struct ubus_context *ctx, struct ubus_object *obj,
			  struct ubus_request_data *req, const char *method,
			  struct blob_attr *msg)
{
	struct geolocation_result *l = &current_location;

	blob_buf_init(&b, 0);
	blobmsg_add_double(&b, "latitude", l->latitude);
	blobmsg_add_double(&b, "longitude", l->longitude);
	blobmsg_add_double(&b, "accuracy", l->accuracy);
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

static const struct ubus_method clocate_methods[] = {
	UBUS_METHOD_NOARG("get_location", clocate_ubus_get_location),
};

static struct ubus_object_type clocate_obj_type =
	UBUS_OBJECT_TYPE("clocate", clocate_methods);

static struct ubus_object clocate_obj = {
	.name = "clocate",
	.type = &clocate_obj_type,
	.methods = clocate_methods,
	.n_methods = ARRAY_SIZE(clocate_methods),
};

void clocate_ubus_init(struct ubus_context *ctx)
{
	ubus_add_object(ctx, &clocate_obj);
}
