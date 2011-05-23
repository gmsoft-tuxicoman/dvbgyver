#
#  feedhunter : Automated satellite feed hunter
#  Copyright (C) 2011 Guy Martin <gmsoft@tuxicoman.be>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

AC_PREREQ(2.61)
AC_INIT([feedhunter], [svn], [Guy Martin <gmsoft@tuxicoman.be>])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

AC_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_DEFUN([AC_PROG_CXX], [])
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([arpa/inet.h fcntl.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
