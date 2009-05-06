# libnetworkd autoconf input
# $Id: configure.ac 42 2008-04-01 14:11:08Z oxff $
# (c) 2007 by Georg 'oxff' Wicherski, <georg-wicherski@pixel-house.net>

AC_PREREQ(2.59)
AC_INIT([libnetworkd], [1.1.0], [georg-wicherski@pixel-house.net])
AM_INIT_AUTOMAKE([libnetworkd], [1.1.0])

AC_REVISION([$Id: configure.ac 42 2008-04-01 14:11:08Z oxff $])

VERSION="1.1.0"
AC_SUBST(VERSION)


AC_PROG_CXX
AC_LANG_CPLUSPLUS

AM_PROG_LIBTOOL

AC_PROG_MAKE_SET

AC_CHECK_FUNCS(daemon)


AC_CHECK_LIB(udns, dns_init)


AC_OUTPUT([
	Makefile
	libnetworkd.pc
	src/Makefile
	])
