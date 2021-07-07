#ifndef __CLOCATE_NL80211_H
#define __CLOCATE_NL80211_H

#include <netlink/genl/ctrl.h>

#include "clocate.h"

struct scan_result {
	unsigned char bssid[6];
	char ssid[33];
	int signal;
};

struct scan_results {
	int result_count;
	struct scan_result *results;
};

#ifdef LIBNL_TINY

#define NL_AUTO_PORT	0

static inline int nl_send_auto(struct nl_sock *sk, struct nl_msg *msg)
{
	return nl_send_auto_complete(sk, msg);
}
#endif

int perform_scan( struct scan_results *results, const char *ifname);
int get_wireless_interfaces(struct clocate_interfaces *results);

#endif