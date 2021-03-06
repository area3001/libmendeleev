# libmendeleev package version number, (as distinct from shared library version)
# An odd micro number indicates in-progress development from Git
# An even micro number indicates a released version
#
# Making a point release:
# - increase libmendeleev_version_micro to the next even number
#
# After the release:
# - increase libmendeleev_version_minor to the next odd number
#
# Take care to update the libtool versioning when required (LIBMODBUS_LD_*).
# http://www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html
#
m4_define([libmendeleev_version_major], [1])
m4_define([libmendeleev_version_minor], [0])
m4_define([libmendeleev_version_micro], [0])

m4_define([libmendeleev_release_status],
    [m4_if(m4_eval(libmendeleev_version_minor % 2), [1], [snapshot], [release])])

m4_define([libmendeleev_version],
    [libmendeleev_version_major.libmendeleev_version_minor.libmendeleev_version_micro])

AC_PREREQ([2.63])
AC_INIT([libmendeleev],
        [libmendeleev_version],
        [https://github.com/area3001/libmendeleev/issues],
        [limendeleev],
        [http://area3001.com/])
AC_CONFIG_SRCDIR([src/mendeleev.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([check-news foreign 1.11 -Wall -Wno-portability silent-rules tar-pax subdir-objects])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])

LIBMENDELEEV_VERSION_MAJOR=libmendeleev_version_major
LIBMENDELEEV_VERSION_MINOR=libmendeleev_version_minor
LIBMENDELEEV_VERSION_MICRO=libmendeleev_version_micro
LIBMENDELEEV_VERSION=libmendeleev_version
AC_SUBST(LIBMENDELEEV_VERSION_MAJOR)
AC_SUBST(LIBMENDELEEV_VERSION_MINOR)
AC_SUBST(LIBMENDELEEV_VERSION_MICRO)
AC_SUBST(LIBMENDELEEV_VERSION)

# ABI version
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
LIBMENDELEEV_LD_CURRENT=6
LIBMENDELEEV_LD_REVISION=0
LIBMENDELEEV_LD_AGE=1
LIBMENDELEEV_LT_VERSION_INFO=$LIBMENDELEEV_LD_CURRENT:$LIBMENDELEEV_LD_REVISION:$LIBMENDELEEV_LD_AGE
AC_SUBST(LIBMENDELEEV_LT_VERSION_INFO)

AC_CANONICAL_HOST

# OS check
os_qnx="true"
AM_CONDITIONAL(OS_QNX, test "$os_qnx" = "true")

LT_INIT([disable-static win32-dll pic-only])
AC_CHECK_HEADERS([ \
    arpa/inet.h \
    byteswap.h \
    errno.h \
    fcntl.h \
    limits.h \
    linux/serial.h \
    netdb.h \
    netinet/in.h \
    netinet/tcp.h \
    sys/ioctl.h \
    sys/params.h \
    sys/socket.h \
    sys/time.h \
    sys/types.h \
    termios.h \
    time.h \
    unistd.h \
])

# Checks for library functions.
AC_CHECK_FUNCS([accept4 getaddrinfo gettimeofday inet_ntoa select socket strerror strlcpy])

# Required for bswap
AC_C_INLINE

# libtool
AC_PROG_CXX

# Various types
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Check for RS485 support (Linux kernel version 2.6.28+)
AC_CHECK_DECLS([TIOCSRS485], [], [], [[#include <sys/ioctl.h>]])
# Check for RTS flags
AC_CHECK_DECLS([TIOCM_RTS], [], [], [[#include <sys/ioctl.h>]])

# Wtype-limits is not supported by gcc 4.2 (default on recent Mac OS X)
my_CFLAGS="-Wall \
-Wmissing-declarations -Wmissing-prototypes \
-Wnested-externs -Wpointer-arith \
-Wpointer-arith -Wsign-compare -Wchar-subscripts \
-Wstrict-prototypes -Wshadow \
-Wformat-security"
AC_SUBST([my_CFLAGS])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
        Makefile
        src/Makefile
        src/mendeleev-version.h
        libmendeleev.pc
])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE $VERSION
        ===============

        prefix:                 ${prefix}
        sysconfdir:             ${sysconfdir}
        libdir:                 ${libdir}
        includedir:             ${includedir}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
])
