#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#ifndef _STRFTIME_MICRO_
#define _STRFTIME_MICRO_

size_t strftime_micro(char *buf, size_t size, const char *format, const struct timeval *tv);

#endif