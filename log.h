#ifndef __CLOCATE_LOG_H
#define __CLOCATE_LOG_H

#include <stdio.h>

#define LOG_ERR_RET(_ret)   fprintf(stderr, "%s: %ld\n", __func__, (long)(_ret));

#endif
