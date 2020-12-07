#ifndef __CLOCATE_UBUS_H
#define __CLOCATE_UBUS_H

#include <libubus.h>

void clocate_ubus_init(struct ubus_context *ctx);

void clocate_ubus_save_location(struct geolocation_result *location);

#endif
