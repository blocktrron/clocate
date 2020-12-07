#ifndef __CLOCATE_DAEMON_H
#define __CLOCATE_DAEMON_H

#include "clocate.h"

struct daemon_config {
	int loc_update_interval;
	char *provider;
};

struct clocated {
	struct daemon_config config; 
};

void clocated_apply_config();

#endif

