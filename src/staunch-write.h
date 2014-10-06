/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */


/* write the link data */
extern int
write_symlink_data (const char *link, const char *id, const char *path);

/* write all the link data */
extern int
write_all_symlink_data (const char *link, const char *id, const char *path,
			const char *target);
