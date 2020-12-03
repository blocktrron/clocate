
#ifndef __WLOCATE_H
#define __WLOCATE_H

struct geolocation_result {
	double latitude;
	double longitude;
	double accuracy;

	time_t timestamp;
};

#endif
