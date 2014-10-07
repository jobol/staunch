/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <unistd.h>

#if !defined(NO_SECURITY_MANAGER)
#include <security-manager.h>
#endif

#include "staunch-common.h"
#include "staunch-read.h"

const char STAUNCH_IDENTITY[] = "staunch";

/* the staunch launcher process */
/* secure launching of an application */
int
main (int argc, char **argv, char **env)
{
  int sts;

  /* get the data */
  sts = read_symlink_data (argv[0], 1);
  if (sts)
    return 1;

#if defined(MAKE_EXEC_CHECK)
  /* check executable status. This is not mandatory because else execve will fail. */
  sts = is_an_executable (read_app_path);
  if (!sts)
    message (LOG_CRIT, "Error, can't exec %s", read_app_path);
  if (sts != 1)
    return 1;
#endif

#if !defined(NO_SECURITY_MANAGER)
  /* prepare environment */
  sts = security_manager_prepare_app (read_app_id);
  if (sts != SECURITY_MANAGER_SUCCESS)
    {
      message (LOG_CRIT, "Error %d while setting security environment", sts);
      return 1;
    }
#endif

  /* change the application name on option */
#if defined(SET_REAL_BINARY_NAME)
  /* set true name of the true executable */
  argv[0] = read_app_path;
#elif defined(SET_APPLICATION_NAME)
  /* set the base name of the true executable */
  /* basename isn't used because it may change the path */
  argv[0] = strrchr (read_app_path, '/');
  argv[0] = argv[0] == NULL ? read_app_path : argv[0] + 1;
#endif

  /* launch */
  execve (read_app_path, argv, env);
  message (LOG_CRIT, "Error when executing %s: %m", read_app_path);
  return 1;
}
