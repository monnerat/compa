dnl Process this file with autoconf to produce a configure script.

AC_INIT([compa], [0.99.1])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_HEADER_STDC
GLIB_GSETTINGS
AC_CHECK_PROG(XGETTEXT, [xgettext] [1])
AC_CHECK_PROG(MSGMERGE, [msgmerge] [1])
AC_CHECK_PROG(INTLTOOL_EXTRACT, [intltool-extract] [1])


GETTEXT_PACKAGE="compa"
AC_SUBST(GETTEXT_PACKAGE)

AC_ARG_ENABLE([werror],
	      [AS_HELP_STRING([--enable-werror], [Use -Werror @<:@yes@:>@])],
	      [enable_werror=yes])
AM_CONDITIONAL([ENABLE_WERROR], [test "$enable_werror" = yes])

PKG_CHECK_MODULES(COMPA, [libmatepanelapplet-4.0 >= 1.5.0])
LIBMATE_PANEL_APPLET_DIR="${datadir}/mate-panel/applets"
AC_SUBST(LIBMATE_PANEL_APPLET_DIR)

AC_ARG_WITH(pkgdatadir,
	    AC_HELP_STRING([--with-pkgdatadir=DIR, package data path]), 
	    [pkgdatadir=$withval],
	    [pkgdatadir="${datadir}/mate-panel/compa"]
)
AS_AC_EXPAND(PKGDATADIR, $pkgdatadir)
AC_DEFINE_UNQUOTED([PKGDATADIR], ["$PKGDATADIR"], [Package data path])

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.4.0])
DBUS_SERVICES_DIR=`${PKG_CONFIG} --variable=session_bus_services_dir dbus-1`
AC_SUBST(DBUS_SERVICES_DIR)

AC_SEARCH_LIBS([gettext], [intl],, [
	AC_MSG_ERROR([gettext support not found])])

AS_AC_EXPAND(PANELLIBEXECDIR, ${libexecdir}/mate-panel)

AS_AC_EXPAND(LOCALEDIR, $localedir)
AC_DEFINE_UNQUOTED([LOCALEDIR], ["$LOCALEDIR"], [Locale directory])


AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/org.mate.panel.applet.compaFactory.service
	src/org.mate.panel.compa.mate-panel-applet
	src/org.mate.panel.applet.compa.gschema.xml
	lang/Makefile
	compa.spec
])

AC_OUTPUT
