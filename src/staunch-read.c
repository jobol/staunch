/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <attr/xattr.h>

#include "staunch-common.h"
#include "staunch-read.h"

static const char STAUNCH_READ_FORMAT[] = "staunch-1.0 %n%*s%n ";

static char buffer[STAUNCH_MAX_LENGTH];
static char target[PATH_MAX];

const char *read_app_id;
const char *read_app_path;
const char *read_target;

/* parse data of the buffer */
static int
parse_buffer_data (int verbose)
{
  int pos1, pos2;

  /* parse the buffer data */
  pos1 = pos2 = 0;
  sscanf (buffer, STAUNCH_READ_FORMAT, &pos1, &pos2);
  if (!pos1 || !pos2)
    {
      if (verbose)
        message (LOG_CRIT, "Error while buffer \"%s\"", buffer);
      return -1;
    }

  /* deduce the data */
  read_app_id = buffer + pos1;
  read_app_path = buffer + pos2 + 1;
  buffer[pos2] = 0;
  return 0;
}

/* read the link data */
int
read_symlink_data (const char *path, int verbose)
{
  ssize_t len;

  /* read the link data */
  len = lgetxattr (path, STAUNCH_XATTR_NAME, buffer, sizeof buffer - 1);
  if (len < 0)
    {
      if (verbose)
        message (LOG_CRIT, "Error while reading attribute %s of %s: %m",
	       STAUNCH_XATTR_NAME, path);
      return -1;
    }
  buffer[len] = 0;

  return parse_buffer_data (verbose);
}

/* read all the link data */
int
read_all_symlink_data (const char *path, int verbose)
{
  ssize_t len;

  len = readlink (path, target, sizeof target - 1);
  if (len < 0)
    {
      if (verbose)
        message (LOG_CRIT, "Error while reading link value of %s: %m", path);
      return -1;
    }
  target[len] = 0;
  read_target = target;

  return read_symlink_data (path, verbose);
}
