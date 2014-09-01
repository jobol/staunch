/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>
#include <grp.h>
#include <sys/types.h>

#include "staunch-messages.h"
#include "staunch-data.h"

static const char usage[] =
	"usage:\n"
	"   %1$s --set [--force] link launcher target [groups]...\n"
	"   %1$s --get link...\n"
	"   %1$s --check link...\n"
;

static int is_valid_target(const char *target)
{
	if (access(target, X_OK)) {
		error("the target %s is not executable", target);
		return 0;
	}
	return 1;
}

static int is_valid_launcher(const char *launcher)
{
	if (access(launcher, X_OK)) {
		error("the launcher %s is not executable", launcher);
		return 0;
	}
	return 1;
}

static int is_decimal(const char *string)
{
	while('0' <= *string && *string <= '9')
		string++;
	return !*string;
}

static gid_t validate_gid(int gid)
{
	if (gid <= 0) {
		error("forbidden gid %d", gid);
		return 0;
	}
	if (gid != (int)(gid_t)gid) {
		error("overflowing gid %d", gid);
		return 0;
	}
	return (gid_t)gid;
}

static gid_t getgroup(const char *grpnam)
{
	struct group *sgrp;

	if (is_decimal(grpnam))
		return validate_gid(atoi(grpnam));

	sgrp = getgrnam(grpnam);
	if (sgrp)
		return validate_gid((int)sgrp->gr_gid);

	error("group %s is not found", grpnam);
	return 0;
}

static const char *getgroupname(gid_t gid)
{
	struct group *sgrp;

	sgrp = getgrgid(gid);
	return sgrp ? sgrp->gr_name : "?";
}

static int set(char **args)
{
	char *link;
	char *launcher;
	struct staunch_data data;
	gid_t gid;
	int i;
	int force;

	/* option force */
	if (!strcmp(*args, "-f") || !strcmp(*args, "--force")) {
		force = 1;
		args++;
	} else {
		force = 0;
	}

	/* record the path */
	link = *args++;
	if (!link) {
		error("link name is missing");
		return 1;
	}

	/* record the launcher */
	launcher = *args++;
	if (!launcher) {
		error("launcher name is missing");
		return 1;
	}

	/* record the target */
	data.target_path = *args++;
	if (!data.target_path) {
		error("target name is missing");
		return 1;
	}

	/* read the groups */
	data.group_count = 0;
	while (*args) {
		gid = getgroup(*args++);
		if (!gid) {
			return 1;
		}
		for (i = 0 ; i < data.group_count && data.groups[i] != gid ; i++);
		if (i == data.group_count)
			data.groups[data.group_count++] = gid;
	}

	/* remove previous link if force is set */
	if (force)
		unlink(link);

	/* validate the target */
	if (!is_valid_target(data.target_path))
		return 1;

	/* validate the launcher */
	if (!is_valid_launcher(launcher))
		return 1;
	
	/* validate the link */
	if (symlink(launcher, link) < 0) {
		error("can't create the symbolic link %s to %s: %m", link, launcher);
		unlink(link);
		return 1;
	}
	/* validate the launcher in the link */
	if (!is_valid_launcher(link)) {
		unlink(link);
		return 1;
	}

	/* write the staunch data attribute */
	if (staunch_data_write(link, &data) < 0) {
		unlink(link);
		return 1;
	}

	return 0;
}

static int get(char **args, int check)
{
	char *link;
	struct staunch_data data;
	char buffer[16384];
	gid_t gid;
	int i;
	int sts;

	/* get the first link */
	link = *args++;
	if (!link) {
		error("link name is missing");
		return 1;
	}

	/* iterate over the links */
	do {
		/* get the data */
		sts = staunch_data_read(link, &data, buffer, sizeof buffer);
		if (!sts) {
			if (!check) {
				/* print the data */
				printf("%s -> %s", link, data.target_path);
				for (i = 0 ; i < data.group_count ; i++) {
					gid = data.groups[i];
					printf(", %d(%s)",(int)gid,getgroupname(gid));
				}
				printf("\n");
			} else {
				is_valid_launcher(link);
				is_valid_target(data.target_path);
				for (i = 0 ; i < data.group_count ; i++)
					validate_gid((int)data.groups[i]);
			}
		}
		/* next */
		link = *args++;
	} while(link);
	return 0;
}

/* the staunch launcher process */
int main(int argc, char **argv, char **env)
{
	char **args;

	/* init the messaging system */
	staunch_messages_init("staunch-manage", 0);

	args = argv+1;
	while (*args) {
		if (!strcmp(*args, "-s") || !strcmp(*args, "--set")) {
			return set(++args);
		} else if (!strcmp(*args, "-g") || !strcmp(*args, "--get")) {
			return get(++args,0);
		} else if (!strcmp(*args, "-c") || !strcmp(*args, "--check")) {
			return get(++args,1);
		} else {
			break;
		}
	}
	error(usage, basename(argv[0]));
	return 1;
}

