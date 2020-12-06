#ifndef __CLOCATE_NL80211_H
#define __CLOCATE_NL80211_H

#include <netlink/genl/ctrl.h>

#include "clocate.h"

#ifdef LIBNL_TINY

#define NL_AUTO_PORT	0

static inline int nl_send_auto(struct nl_sock *sk, struct nl_msg *msg)
{
	return nl_send_auto_complete(sk, msg);
}
#endif

struct if_results {
	char *buf;
	size_t count;
};

int perform_scan( struct scan_results *results, const char *ifname);
int get_wireless_interfaces(struct if_results *results);

#endif