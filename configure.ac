dnl Process this file with autoconf to produce a configure script.

AC_INIT([compa], [0.99])
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

PKG_CHECK_MODULES(COMPA, [libmatepanelapplet-4.0 >= 1.5.0])
LIBMATE_PANEL_APPLET_DIR="${datadir}/mate-panel/applets"
AC_SUBST(LIBMATE_PANEL_APPLET_DIR)

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.4.0])
DBUS_SERVICES_DIR=`${PKG_CONFIG} --variable=session_bus_services_dir dbus-1`
AC_SUBST(DBUS_SERVICES_DIR)

AC_SEARCH_LIBS([gettext], [intl],, [
	AC_MSG_ERROR([gettext support not found])])

AS_AC_EXPAND(LIBEXECDIR, $libexecdir)
AC_SUBST(LIBEXECDIR)

PANELLIBEXECDIR="${libexecdir}/mate-panel"
AC_SUBST(PANELLIBEXECDIR)

AS_AC_EXPAND(LOCALEDIR, $localedir)
AC_SUBST([LOCALEDIR])
AC_DEFINE_UNQUOTED([LOCALEDIR],["$LOCALEDIR"],[Locale directory])


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
