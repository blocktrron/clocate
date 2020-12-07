#include <stdio.h>
#include <string.h>
#include <netlink/genl/genl.h>
#include <net/if.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libubox/blobmsg.h>
#include <libubox/uloop.h>
#include <libubox/utils.h>
#include <libubus.h>

#include "nl80211.h"
#include "clocate.h"
#include "ubus.h"
#include "daemon.h"

struct geolocation_result current_location;
struct ubus_context *ubus_ctx;
struct clocated clocated;

static void
clocated_update_location(struct uloop_timeout *t)
{
	struct locator_config loc_config;

	memset(&loc_config, 0, sizeof(loc_config));

	loc_config.provider = get_geolocation_provider(clocated.config.provider);
	if (!loc_config.provider) {
		fprintf(stderr, "No provider %s\n", clocated.config.provider);
		goto out;
	}

	get_wireless_interfaces(&loc_config.interfaces);

	if (loc_config.interfaces.count == 0) {
		fprintf(stderr, "No wireless interfaces available\n");
		goto out;
	}

	start_geolocation(&loc_config, &current_location);
	clocate_ubus_save_location(&current_location);

out:
	if (loc_config.interfaces.buf)
		free(loc_config.interfaces.buf);

	uloop_timeout_set(t, clocated.config.loc_update_interval * 1000);
}

static struct uloop_timeout location_timer = {
	.cb = clocated_update_location,
};

static void clocated_load_default_config()
{
	clocated.config.loc_update_interval = 60;
	clocated.config.provider = "mozilla";
}

void clocated_apply_config()
{
	uloop_timeout_cancel(&location_timer);
	uloop_timeout_set(&location_timer, 0);
}

int main(int argc, char *argv[]) {
	int ret;
	char c;

	memset(&clocated, 0, sizeof(clocated));

	uloop_init();

	ubus_ctx = ubus_connect(NULL);
	if (!ubus_ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		ret = -ENOSYS;
		goto out;
	}

	clocated_load_default_config();
	clocated_apply_config();

	clocate_ubus_init(ubus_ctx);
	ubus_add_uloop(ubus_ctx);

	uloop_run();
	uloop_done();

out:
	return ret;
}
