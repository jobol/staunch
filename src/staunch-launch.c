/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <grp.h>

#include "staunch-messages.h"
#include "staunch-data.h"

/* the staunch launcher process */
int main(int argc, char **argv, char **env)
{
	char buffer[16384];
	gid_t groups[1024];
	struct staunch_data data;
	int sts;
	gid_t gid;
	int nrgrp;
	int gmod;
	int i;

	/* init the messaging system */
	staunch_messages_init("staunch-launch", 1);

	/* read the security staunch data of the link */
	sts = staunch_data_read(argv[0], &data, buffer, sizeof buffer);
	if (sts) {
		return 1;
	}

	/* add groups if needed */
	if (data.group_count) {

		/* get the current group list */
		nrgrp = getgroups(sizeof groups / sizeof * groups, groups);
		if (nrgrp < 0) {
			error("Error %d while reading group's list: %m\n", -errno);
			return 1;
		}
		gmod = 0;
		while (data.group_count) {
			gid = data.groups[--data.group_count];
			assert(gid);
			i = 0;
			while (gid) {
				if (i < nrgrp) {
					if (groups[i] == (gid_t)gid)
						gid = 0;
					else
						i++;
				} else if (nrgrp == sizeof groups / sizeof * groups) {
					error("Error while adding group to list: too many groups\n");
					return 1;
				} else {
					groups[nrgrp++] = (gid_t)gid;
					gmod = 1;
					gid = 0;
				}
			}
				
		}

		/* set the extended group list */
		if (gmod) {
			sts = setgroups(nrgrp, groups);
			if (sts < 0) {
				error("Error %d while setting group's list: %m\n", -errno);
				return 1;
			}
		}
	}

	/* execute the linked program */
	argv[0] = data.target_path;
	execve(argv[0], argv, env);
	error("Error %d while executing %s: %m\n", -errno, argv[0]);
	return 1;
}

