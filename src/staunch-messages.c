/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <assert.h>

#include "staunch-messages.h"

static enum { uninitialised, normal, syslogging } state = uninitialised;

#define emit(priority,format) {\
		va_list ap; \
		assert(state != uninitialised); \
		va_start(ap, format); \
		if (state == normal) {\
			vfprintf(stderr, format, ap); \
			fprintf(stderr, "\n"); \
		} else \
			vsyslog(priority, format, ap); \
		va_end(ap); \
	}

void staunch_messages_init(const char *identity, int dosyslog)
{
	assert(state == uninitialised);
	if (dosyslog) {
		openlog(identity, LOG_PERROR|LOG_NDELAY, LOG_AUTH);
		state = syslogging;
	} else {
		state = normal;
	}
}

void fatal(const char* format, ...)
{
	emit(LOG_CRIT, format);
	exit(1);
}

void error(const char* format, ...)
{
	emit(LOG_CRIT, format);
}

void warning(const char* format, ...)
{
	emit(LOG_CRIT, format);
}

