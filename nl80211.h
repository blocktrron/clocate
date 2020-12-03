#ifndef __WLOCATE_NL80211_H
#define __WLOCATE_NL80211_H

#define 	MAC2STR(a)   (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define 	MACSTR   "%02x:%02x:%02x:%02x:%02x:%02x"

struct scan_result {
	unsigned char bssid[6];
	char ssid[33];
	int signal;
};

struct scan_results {
	int result_count;
	struct scan_result *results;
};

int perform_scan( struct scan_results *results, const char *ifname);

#endif