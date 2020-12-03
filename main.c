#include <stdio.h>
#include <netlink/genl/genl.h>

#include "nl80211.h"
#include "mozilla.h"
#include "wlocate.h"


int main() {
	struct scan_results results = {};
	struct geolocation_result geolocation_result = {};

	perform_scan(&results, "wlp4s0");
	locate_mozilla(&results, &geolocation_result);
	printf("%f,%f\n", geolocation_result.latitude, geolocation_result.longitude);

	return 0;
}
