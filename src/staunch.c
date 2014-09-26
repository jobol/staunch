/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <attr/xattr.h>
#include <syslog.h>
#include <stdarg.h>
#include <libgen.h>

#if !defined(NO_SECURITY_MANAGER)
#include <security-manager.h>
#endif

static const char usage[] =
  "usage:\n"
  "   %1$s --set [--force] appid target link\n"
  "   %1$s --get link...\n"
  "   %1$s --check link...\n";

static const char STAUNCH_IDENTITY[] = "staunch";
static const char STAUNCH_XATTR_NAME[] = "security.staunch";

static const char STAUNCH_WRITE_FORMAT[] = "staunch-1.0 %s %s";
static const char STAUNCH_READ_FORMAT[] = "staunch-1.0 %n%*s%n ";

static char me[PATH_MAX];
static char buffer[2 * PATH_MAX];
static const char *app_id;
static const char *app_path;
static char target[PATH_MAX];

static void
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
static int
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
static int
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
static int
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

/* parse data of the buffer */
static int
parse_buffer_data ()
{
  int pos1, pos2;

  /* parse the buffer data */
  pos1 = pos2 = 0;
  sscanf (buffer, STAUNCH_READ_FORMAT, &pos1, &pos2);
  if (!pos1 || !pos2)
    {
      message (LOG_CRIT, "Error while buffer \"%s\"", buffer);
      return -1;
    }

  /* deduce the data */
  app_id = buffer + pos1;
  app_path = buffer + pos2 + 1;
  buffer[pos2] = 0;
  return 0;
}

/* set the data to the buffer */
static int
set_buffer_data (const char *id, const char *path)
{
  int len;

  len = snprintf (buffer, sizeof buffer, STAUNCH_WRITE_FORMAT, id, path);
  if (len < 0 || sizeof buffer <= (size_t) len)
    {
      message (LOG_CRIT, "Data too long (%d): id=\"%s\" path=\"%s\"", len, id,
	       path);
      return -1;
    }
  assert (!parse_buffer_data ()
	  && !(strcmp (app_id, id) || strcmp (app_path, path)));
  return len;
}

/* read the link data */
static int
read_symlink_data (const char *path)
{
  ssize_t len;

  assert (is_a_symlink (path) == 1);

  /* read the link data */
  len = lgetxattr (path, STAUNCH_XATTR_NAME, buffer, sizeof buffer - 1);
  if (len < 0)
    {
      message (LOG_CRIT, "Error while reading attribute %s of %s: %m",
	       STAUNCH_XATTR_NAME, path);
      return -1;
    }
  buffer[len] = 0;

  return parse_buffer_data ();
}

/* write the link data */
static int
write_symlink_data (const char *link, const char *id, const char *path)
{
  int sts, len;

  assert (is_a_symlink (link) == 1);

  len = set_buffer_data (id, path);
  if (len < 0)
    return -1;
  sts = lsetxattr (link, STAUNCH_XATTR_NAME, buffer, (size_t) len, 0);
  if (sts)
    {
      message (LOG_CRIT, "Error while writing attribute %s of %s: %m",
	       STAUNCH_XATTR_NAME, link);
      return -1;
    }
  assert (!read_symlink_data (link)
	  && !(strcmp (app_id, id) || strcmp (app_path, path)));
  return 0;
}

/* read all the link data */
static int
read_all_symlink_data (const char *path)
{
  ssize_t len;

  assert (is_a_symlink (path) == 1);

  len = readlink (path, target, sizeof target - 1);
  if (len < 0)
    return -1;
  target[len] = 0;

  return read_symlink_data (path);
}

/* write all the link data */
static int
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

/* set me with the absolute path */
static int
set_me (const char *path)
{
  char *x = realpath (path, me);
  if (x != me)
    {
      message (LOG_CRIT, "can't resolve path %s", path);
      return -1;
    }
  return 0;
}

/* secure launching of an application */
static void
launch (int argc, char **argv, char **env)
{
  int sts;

  assert (is_a_symlink (argv[0]) == 1);

  /* get the data */
  sts = read_symlink_data (argv[0]);
  if (sts)
    exit (1);

#if defined(MAKE_EXEC_CHECK)
  /* check executable status. This is not mandatory because else execve will fail. */
  sts = is_an_executable (app_path);
  if (!sts)
    message (LOG_CRIT, "Error, can't exec %s", app_path);
  if (sts != 1)
    exit (1);
#endif

#if !defined(NO_SECURITY_MANAGER)
  /* prepare environment */
  sts = security_manager_prepare_app (app_id);
  if (sts != SECURITY_MANAGER_SUCCESS)
    {
      message (LOG_CRIT, "Error %d while setting security environment", sts);
      exit (1);
    }
#endif

  /* change the application name on option */
#if defined(SET_REAL_BINARY_NAME)
  /* set true name of the true executable */
  argv[0] = app_path;
#elif defined(SET_APPLICATION_NAME)
  /* set the base name of the true executable */
  /* basename isn't used because it may change the path */
  argv[0] = strrchr (app_path, '/');
  argv[0] = argv[0] == NULL ? app_path : argv[0] + 1;
#endif

  /* launch */
  execve (app_path, argv, env);
  message (LOG_CRIT, "Error when executing %s: %m", app_path);
  exit (1);
}

/* implement the set sub command */
static void
set (int argc, char **argv)
{
  int sts;
  int force;
  const char *id;
  const char *path;
  const char *link;
  char *apath;

  /* set me */
  if (set_me (argv[0]))
    exit (1);

  /* has force flag? */
  force = !strcmp (argv[2], "-f") || !strcmp (argv[2], "--force");

  /* get items */
  if (argc != 5 + force)
    {
      message (LOG_ERR, "bad argument count");
      exit (1);
    }
  id = argv[2 + force];
  path = argv[3 + force];
  link = argv[4 + force];

  /* checking if the path exists and is executable */
  sts = is_existing (path);
  if (sts < 0)
    exit (1);
  if (!sts)
    {
      message (LOG_ERR, "The application %s isn't existing", path);
      exit (1);
    }
  apath = realpath (path, NULL);
  if (apath == NULL)
    {
      message (LOG_ERR, "Failed to get absolute path of %s", path);
      exit (1);
    }
  sts = is_an_executable (apath);
  if (sts < 0)
    exit (1);
  if (!sts)
    {
      message (LOG_ERR, "The application %s isn't executable", apath);
      exit (1);
    }

  /* checking if allowed to overwrite the link */
  sts = is_existing (link);
  if (sts < 0)
    exit (1);
  if (sts)
    {
      if (!force || !is_a_symlink (link))
	{
	  message (LOG_ERR, "Can't overwrite existing %s", link);
	  exit (1);
	}
      /* remove previous link if force is set */
      unlink (link);
    }

  /* write the data to the link */
  sts = write_all_symlink_data (link, id, apath, me);
  if (sts < 0)
    {
      unlink (link);
      exit (1);
    }
}

/* implement the get subcommand */
static void
get (char **links)
{
  int sts;
  while (*links != NULL)
    {
      sts = read_symlink_data (*links);
      if (sts)
	fprintf (stderr, "error %s\n", *links);
      else
	printf ("%s: %s %s\n", *links, app_id, app_path);
      links++;
    }
}

/* implement the check subcommand */
static void
check (char **links)
{
  int quiet;
  int haserror;
  const char *msg;

  quiet = !strcmp (*links, "-q") || !strcmp (*links, "--quiet");
  if (quiet)
    links++;
  haserror = 0;
  while (*links != NULL)
    {
      if (1 != is_existing (*links))
	msg = "ERROR(doesn't exist)";
      else if (1 != is_a_symlink (*links))
	msg = "ERROR(isn't a link)";
      else if (0 != read_all_symlink_data (*links))
	msg = "ERROR(isn't a valid staunch link)";
      else if (0 != strcmp (target, me))
	msg = "ERROR(isn't linked to me)";
      else if (1 != is_existing (app_path))
	msg = "ERROR(app path doesn't exist)";
      else if (1 != is_an_executable (app_path))
	msg = "ERROR(app path isn't executable)";
      else
	msg = NULL;
      if (msg == NULL)
	msg = "valid";
      else
	haserror = 1;
      if (!quiet)
	printf ("%s: %s\n", *links, msg);
      links++;
    }
    exit(haserror);
}

/* launched as manager */
static void
manage (int argc, char **argv)
{
  int help = 0;
  const char *command;

  command = argv[1];
  if (command == NULL)
    {
      help = 1;
    }
  else if (!strcmp (command, "-s") || !strcmp (command, "--set"))
    {
      set (argc, argv);
    }
  else if (!strcmp (command, "-g") || !strcmp (command, "--get"))
    {
      get (argv + 2);
    }
  else if (!strcmp (command, "-c") || !strcmp (command, "--check"))
    {
      if (set_me (argv[0]))
	exit (1);
      check (argv + 2);
    }
  else if (!strcmp (command, "-h") || !strcmp (command, "--help"))
    {
      help = 1;
    }
  else
    {
      help = 2;
    }
  if (help)
    {
      printf (usage, basename (argv[0]));
      exit (help - 1);
    }
}


/* the staunch launcher process */
int
main (int argc, char **argv, char **env)
{
  int sts;

  sts = is_a_symlink (argv[0]);
  if (sts < 0)
    return 1;

  if (sts)
    launch (argc, argv, env);
  else
    manage (argc, argv);

  return 0;
}
