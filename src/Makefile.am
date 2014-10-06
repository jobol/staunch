bin_PROGRAMS = staunch
staunch_SOURCES = staunch.c staunch-common.c staunch-read.c
staunch_CFLAGS = $(SECURITY_MANAGER_CFLAGS) -DNDEBUG
staunch_LDADD = $(SECURITY_MANAGER_LIBS) -lattr

sbin_PROGRAMS = stauncher
stauncher_SOURCES = stauncher.c staunch-common.c staunch-read.c  staunch-write.c
stauncher_CFLAGS = $(SECURITY_MANAGER_CFLAGS) -DNDEBUG -DLAUNCHER=\"$(bindir)/staunch\"
stauncher_LDADD = $(SECURITY_MANAGER_LIBS) -lattr

