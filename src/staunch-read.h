/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

extern const char *read_app_id;
extern const char *read_app_path;
extern const char *read_target;

/* read the link data: 'read_app_id' and 'read_app_path' */
extern int read_symlink_data (const char *path, int verbose);

/* read all the link data: read_app_id, read_app_path, read_target */
extern int read_all_symlink_data (const char *path, int verbose);
