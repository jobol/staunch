/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <attr/xattr.h>

#include "staunch-common.h"
#include "staunch-write.h"

static const char STAUNCH_WRITE_FORMAT[] = "staunch-1.0 %s %s";

/* set the data to the buffer */
static int
set_buffer_data (char *buffer, size_t size, const char *id, const char *path)
{
  int len;

  len = snprintf (buffer, size, STAUNCH_WRITE_FORMAT, id, path);
  if (len < 0 || size <= (size_t) len)
    {
      message (LOG_CRIT, "Data too long (%d): id=\"%s\" path=\"%s\"", len, id,
	       path);
      return -1;
    }
  return len;
}

/* write the link data */
int
write_symlink_data (const char *link, const char *id, const char *path)
{
  int sts, len;
  char buffer[STAUNCH_MAX_LENGTH];

  len = set_buffer_data (buffer, sizeof buffer, id, path);
  if (len < 0)
    return -1;

  sts = lsetxattr (link, STAUNCH_XATTR_NAME, buffer, (size_t) len, 0);
  if (sts)
    {
      message (LOG_CRIT, "Error while writing attribute %s of %s: %m",
	       STAUNCH_XATTR_NAME, link);
      return -1;
    }
  return 0;
}

/* write all the link data */
int
write_all_symlink_data (const char *link, const char *id, const char *path,
			const char *target)
{
  int sts;

  /* create the link */
  sts = symlink (target, link);
  if (sts < 0)
    {
      message (LOG_CRIT, "can't create the symbolic link %s to %s: %m", link,
	       target);
    }
  else
    {
      /* write the link */
      sts = write_symlink_data (link, id, path);
    }
  return sts;
}
