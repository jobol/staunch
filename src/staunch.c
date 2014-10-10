/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <unistd.h>
#include <sys/prctl.h>
#include <linux/capability.h>

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
  unsigned long capa;
  struct __user_cap_header_struct caphead;
  struct __user_cap_data_struct capdata[2];

  /* get the data */
  sts = read_symlink_data (argv[0], 1);
  if (sts)
    return 1;

#if defined(MAKE_EXEC_CHECK)
  /* check executable status. */
  /* This is not mandatory because else execve will fail. */
  sts = is_an_executable (read_app_path);
  if (!sts)
    message (LOG_CRIT, "Error, can't exec %s", read_app_path);
  if (sts != 1)
    return 1;
#endif

#if !defined(NO_SECURITY_MANAGER)
#if defined(USE_SECURITY_MANAGER_PREPARE_APP)
  /* prepare environment */
  sts = security_manager_prepare_app (read_app_id);
  if (sts != SECURITY_MANAGER_SUCCESS)
    {
      message (LOG_CRIT, "Error %d while setting security environment", sts);
      return 1;
    }
#else
  /* prepare groups */
  sts = security_manager_set_process_groups_from_appid (read_app_id);
  if (sts != SECURITY_MANAGER_SUCCESS)
    {
      message (LOG_CRIT, "Error %d while setting security groups", sts);
      return 1;
    }
  /* prepare environment */
  sts = security_manager_set_process_label_from_appid (read_app_id);
  if (sts != SECURITY_MANAGER_SUCCESS)
    {
      message (LOG_CRIT, "Error %d while setting security context", sts);
      return 1;
    }

  /* drop read/write/execute when creating files and directories */
  umask(077);

  /* clears the capability bound set */
  for (capa = 0 ; capa <= CAP_LAST_CAP ; capa++)
    {
      sts = prctl(PR_CAPBSET_DROP, capa);
      if (sts)
        {
          message (LOG_CRIT, "Error while clearing capability bounding set (%d): %m", (int)capa);
          return 1;
        }
    }

  /* drop any current capability */
  caphead.version = _LINUX_CAPABILITY_VERSION_3;
  caphead.pid = 0;
  capdata[0].effective = capdata[0].permitted = capdata[0].inheritable = 0;
  capdata[1].effective = capdata[1].permitted = capdata[1].inheritable = 0;
  sts = capset(&caphead, capdata);
  if (sts)
    {
      message (LOG_CRIT, "Error while removing capabitilies: %m");
      return 1;
    }

#endif
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
