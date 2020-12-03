#ifndef __WLOCATE_MOZILLA
#define __WLOCATE_MOZILLA

#include "nl80211.h"
#include "mozilla.h"
#include "wlocate.h"

int locate_mozilla(struct scan_results *results, struct geolocation_result *geolocation);

#endif