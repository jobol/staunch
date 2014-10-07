/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#include "staunch-common.h"
#include "staunch-read.h"
#include "staunch-write.h"

static const char usage[] =
  "usage:\n"
  "   %1$s set [--force] appid target link\n"
  "   %1$s get link...\n" "   %1$s check [--quiet] link...\n";

const char STAUNCH_IDENTITY[] = "stauncher";

static char launcher[] = LAUNCHER;

/* checking the launcher */
static int
check_launcher ()
{
  int sts;

  sts = is_an_executable (launcher);
  if (sts == 0)
    message (LOG_ERR, "launcher %s is not executable");
  return sts == 1 ? 0 : -1;
}

/* implement the set sub command */
static int
set (int argc, char **argv)
{
  int sts;
  int force;
  const char *id;
  const char *path;
  const char *link;
  char *apath;

  /* check the launcher */
  if (check_launcher ())
    return 1;

  /* has force flag? */
  force = !strcmp (argv[2], "-f") || !strcmp (argv[2], "--force");

  /* get items */
  if (argc != 5 + force)
    {
      message (LOG_ERR, "bad argument count");
      return 1;
    }
  id = argv[2 + force];
  path = argv[3 + force];
  link = argv[4 + force];

  /* checking if the path exists and is executable */
  sts = is_existing (path);
  if (sts < 0)
    return 1;
  if (!sts)
    {
      message (LOG_ERR, "The application %s isn't existing", path);
      return 1;
    }
  apath = realpath (path, NULL);
  if (apath == NULL)
    {
      message (LOG_ERR, "Failed to get absolute path of %s", path);
      return 1;
    }
  sts = is_an_executable (apath);
  if (sts < 0)
    return 1;
  if (!sts)
    {
      message (LOG_ERR, "The application %s isn't executable", apath);
      return 1;
    }

  /* checking if allowed to overwrite the link */
  sts = is_existing (link);
  if (sts < 0)
    return 1;
  if (sts)
    {
      if (!force || !is_a_symlink (link))
	{
	  message (LOG_ERR, "Can't overwrite existing %s", link);
	  return 1;
	}
      /* remove previous link if force is set */
      unlink (link);
    }

  /* write the data to the link */
  sts = write_all_symlink_data (link, id, apath, launcher);
  if (sts < 0)
    {
      unlink (link);
      return 1;
    }

  /* control the setting */
  sts = read_all_symlink_data (link, 1);
  if (sts != 0)
    message (LOG_CRIT, "Unable to read the written data: %m");
  else
    {
      if (strcmp (id, read_app_id))
	{
	  message (LOG_CRIT,
		   "application id doesn't match: written=%s, read=%s", id,
		   read_app_id);
	  sts = -1;
	}
      if (strcmp (apath, read_app_path))
	{
	  message (LOG_CRIT,
		   "application path doesn't match: written=%s, read=%s",
		   apath, read_app_path);
	  sts = -1;
	}
      if (strcmp (launcher, read_target))
	{
	  message (LOG_CRIT, "link target doesn't match: written=%s, read=%s",
		   launcher, read_target);
	  sts = -1;
	}
      if (sts == 0)
	return 0;
    }

  /* error */
  unlink (link);
  return 1;
}

/* implement the get subcommand */
static int
get (char **links)
{
  int sts;
  int quiet;

  quiet = !strcmp (*links, "-q") || !strcmp (*links, "--quiet");
  if (quiet)
    links++;

  while (*links != NULL)
    {
      sts = read_symlink_data (*links, 0);
      if (!sts)
	printf ("%s: %s %s\n", *links, read_app_id, read_app_path);
      else if (!quiet)
	fprintf (stderr, "error %s\n", *links);
      links++;
    }
  return 0;
}

/* implement the check subcommand */
static int
check (char **links)
{
  int quiet;
  int haserror;
  const char *msg;

  quiet = !strcmp (*links, "-q") || !strcmp (*links, "--quiet");
  if (quiet)
    links++;

  /* check the links */
  haserror = 0;
  while (*links != NULL)
    {
      if (1 != is_existing (*links))
	msg = "ERROR(doesn't exist)";
      else if (1 != is_a_symlink (*links))
	msg = "ERROR(isn't a link)";
      else if (0 != read_all_symlink_data (*links, 0))
	msg = "ERROR(isn't a valid staunch link)";
      else if (0 != strcmp (read_target, launcher))
	msg = "ERROR(isn't linked to launcher)";
      else if (1 != is_existing (read_app_path))
	msg = "ERROR(app path doesn't exist)";
      else if (1 != is_an_executable (read_app_path))
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

  /* check the launcher */
  if (check_launcher ())
    haserror = 1;

  return haserror;
}

/* the staunch manager process */
int
main (int argc, char **argv, char **env)
{
  int help = 0;
  const char *command;

  command = argv[1];
  if (command == NULL)
    {
      help = 1;
    }
  else if (!strcmp (command, "set"))
    {
      return set (argc, argv);
    }
  else if (!strcmp (command, "get"))
    {
      return get (argv + 2);
    }
  else if (!strcmp (command, "check"))
    {
      return check (argv + 2);
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
      return help - 1;
    }
  return 0;
}
