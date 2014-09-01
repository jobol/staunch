/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

extern const char STAUNCH_MESSAGE_IDENTITY[];

void staunch_messages_init(const char *identity, int dosyslog);

void fatal(const char* format, ...);

void error(const char* format, ...);

void warning(const char* format, ...);

