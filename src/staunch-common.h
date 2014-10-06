/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

#include <syslog.h>

#define STAUNCH_MAX_LENGTH  8192

extern const char STAUNCH_IDENTITY[];
extern const char STAUNCH_XATTR_NAME[];

/* prompt a message */
extern void message (int priority, const char *format, ...);

/* check if path is existing */
extern int is_existing (const char *path);

/* check if path is a symbolic link */
extern int is_a_symlink (const char *path);

/* check if path is executable */
extern int is_an_executable (const char *path);
