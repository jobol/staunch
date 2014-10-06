/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/xattr.h>

#include "staunch-common.h"

const char STAUNCH_XATTR_NAME[] = XATTR_SECURITY_PREFIX "staunch";

void
message (int priority, const char *format, ...)
{
  va_list ap;
  static int init = 0;

  if (!init)
    {
      openlog (STAUNCH_IDENTITY, LOG_PERROR | LOG_NDELAY, LOG_AUTH);
      init = 1;
    }
  va_start (ap, format);
  vsyslog (priority, format, ap);
  va_end (ap);
}

/* check if path is existing */
int
is_existing (const char *path)
{
  int sts;

  sts = access (path, F_OK);
  if (sts && errno != ENOENT && errno != EACCES)
    {
      message (LOG_CRIT, "can't get access of %s: %m", path);
      return -1;
    }

  return sts == 0;
}

/* check if path is a symbolic link */
int
is_a_symlink (const char *path)
{
  struct stat st;
  int sts;

  sts = lstat (path, &st);
  if (sts)
    {
      message (LOG_CRIT, "can't get status of %s: %m", path);
      return -1;
    }

  return S_ISLNK (st.st_mode);
}

/* check if path is executable */
int
is_an_executable (const char *path)
{
  int sts;

  sts = access (path, X_OK);
  if (sts && errno != EACCES)
    {
      message (LOG_CRIT, "can't get access of %s: %m", path);
      return -1;
    }

  return sts == 0;
}
