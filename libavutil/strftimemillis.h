#ifndef AVUTIL_STRFTIMEMILLIS_H
#define AVUTIL_STRFTIMEMILLIS_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

size_t strftime_millis(char *ptr, size_t maxsize, const char *format, const struct timeval *tv);
#endif
