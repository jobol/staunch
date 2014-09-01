/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

enum {
	staunch_data_max_count_of_groups = 1024
};

struct staunch_data {
	char *target_path;
	int group_count;
	gid_t groups[staunch_data_max_count_of_groups];
};

int staunch_data_read(const char *path, struct staunch_data *data, char *buffer, size_t bufferlength);

#if !defined(ONLY_READ)
int staunch_data_write(const char *path, const struct staunch_data *data);
#endif

