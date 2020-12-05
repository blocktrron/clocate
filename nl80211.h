#ifndef __WLOCATE_NL80211_H
#define __WLOCATE_NL80211_H

#include <netlink/genl/ctrl.h>

#define 	MAC2STR(a)   (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define 	MACSTR   "%02x:%02x:%02x:%02x:%02x:%02x"

#ifdef LIBNL_TINY

#define NL_AUTO_PORT	0

static inline int nl_send_auto(struct nl_sock *sk, struct nl_msg *msg)
{
	return nl_send_auto_complete(sk, msg);
}
#endif

struct scan_result {
	unsigned char bssid[6];
	char ssid[33];
	int signal;
};

struct scan_results {
	int result_count;
	struct scan_result *results;
};

struct if_results {
	char *buf;
	size_t count;
};

int perform_scan( struct scan_results *results, const char *ifname);
int get_wireless_interfaces(struct if_results *results);

#endif