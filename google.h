#ifndef __WLOCATE_GOOGLE
#define __WLOCATE_GOOGLE

#include "nl80211.h"
#include "wlocate.h"

int locate_google(struct scan_results *results, struct geolocation_result *geolocation, char *api_key);

#endif