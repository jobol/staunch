/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <alloca.h>

#include "staunch-data.h"

#include "staunch-messages.h"

/*
The content of the security attribute security.staunch is:
	security.staunch = "staunch-1.0\0" PATH [ '\0' GID [ ' ' GID ]... ]
*/
static const char STAUNCH_XATTR_NAME[] = "security.staunch";
static const char STAUNCH_XATTR_PREFIX[] = "staunch-1.0";


int staunch_data_read(const char *path, struct staunch_data *data, char *buffer, size_t bufferlength)
{
	int sts;
	int len;
	int offset;
	int gid;
	char car;

	assert(path);
	assert(data);
	assert(buffer);
	assert(bufferlength > 0);

	/* read the security staunch key of the link */
	len = lgetxattr(path, STAUNCH_XATTR_NAME, buffer, bufferlength - 1);
	if (len < 0) {
		error("Error %d while reading attribute %s of %s: %m",
			-errno, STAUNCH_XATTR_NAME, path);
		return -1;
	}
	buffer[len] = 0;

	/* scan the attribute */

	/* check the prefix */
	if (strcmp(buffer, STAUNCH_XATTR_PREFIX)) {
		error("Error of format for attribute: isn't prefixed by '%s'",
			STAUNCH_XATTR_PREFIX);
		errno = ENOEXEC;
		return -1;
	}

	/* get the path */
	data->target_path = buffer + sizeof STAUNCH_XATTR_PREFIX;

	/* get the offset of eventual gids */
	data->group_count = 0;
	offset = (int)(strlen(data->target_path) + sizeof STAUNCH_XATTR_PREFIX);
	if (offset != len) {
		/* read the extended groups */
		do {
			car = buffer[++offset];
			gid = 0;
			while ('0' <= car && car <= '9') {
				gid = 10 * gid + (int)(car - '0');
				car = buffer[++offset];
			}
			if (gid != (int)(gid_t)gid) {
				error("Error of group id: overfow of %d", gid);
				return -1;
			}
			if (gid) {
				if (data->group_count == (int)(sizeof data->groups / sizeof * data->groups)) {
					error("Error while adding group to list: too many groups");
					return -1;
				}
				data->groups[data->group_count++] = (gid_t)gid;
			}
				
		} while (car);
	}
	return 0;
}

#if !defined(ONLY_READ)

#include "itoa.h"

int staunch_data_write(const char *path, const struct staunch_data *data)
{
	size_t plen;
	size_t alen;
	size_t len;
	char *buffer;
	int igrp;
	int sts;

	assert(path);
	assert(data);
	assert(data->target_path);
	assert(data->group_count >= 0);

	plen = strlen(data->target_path);
	len = plen + sizeof STAUNCH_XATTR_PREFIX;
	alen = len + (size_t)(data->group_count * 12);
		/* 12 is enought for 32 bit integers and at least 1 character more */
	buffer = alloca(alen);
	memcpy(buffer, STAUNCH_XATTR_PREFIX, sizeof STAUNCH_XATTR_PREFIX);
	memcpy(buffer + sizeof STAUNCH_XATTR_PREFIX, data->target_path, plen);
	for (igrp = 0 ; igrp < data->group_count ; igrp++) {
		assert(len < alen);
		buffer[len++] = igrp ? ' ' : (char)0;
		assert(len + 10 < alen);
		len += _utoa((unsigned)data->groups[igrp], buffer+len);
	}
	assert(len <= alen);

	sts = lsetxattr(path, STAUNCH_XATTR_NAME, buffer, len, 0);
	if (sts < 0) {
		error("Error %d while writing attribute %s of %s: %m",
			-errno, STAUNCH_XATTR_NAME, path);
		return -1;
	}
	return 0;
}

#endif

