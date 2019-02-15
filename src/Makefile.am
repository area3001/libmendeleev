EXTRA_DIST =
lib_LTLIBRARIES = libmendeleev.la

AM_CPPFLAGS = \
    -include $(top_builddir)/config.h \
    -DSYSCONFDIR=\""$(sysconfdir)"\" \
    -DLIBEXECDIR=\""$(libexecdir)"\" \
    -I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS}

libmendeleev_la_SOURCES = \
        mendeleev.c \
        mendeleev.h \
        mendeleev-private.h \
        mendeleev-rtu.c \
        mendeleev-rtu.h \
        mendeleev-rtu-private.h \
        mendeleev-version.h

libmendeleev_la_LDFLAGS = -no-undefined \
        -version-info $(LIBMENDELEEV_LT_VERSION_INFO)

# Header files to install
libmendeleevincludedir = $(includedir)/mendeleev
libmendeleevinclude_HEADERS = mendeleev.h mendeleev-version.h mendeleev-rtu.h

DISTCLEANFILES = mendeleev-version.h
EXTRA_DIST += mendeleev-version.h.in
CLEANFILES = *~
