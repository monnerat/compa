## Process this file with automake to produce Makefile.in

AM_CPPFLAGS		=						\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	$(COMPA_CFLAGS)

AM_CFLAGS		=	-Wall -g

panellibexecdir		=	@PANELLIBEXECDIR@

panellibexec_PROGRAMS	=	compa-applet

compa_applet_SOURCES	=	main.c

compa_applet_LDFLAGS	=	-Wl,--export-dynamic

compa_applet_LDADD	=	$(COMPA_LIBS)


servicedir		=	@DBUS_SERVICES_DIR@
service_DATA		=	org.mate.panel.applet.compaFactory.service


appletdir		=	@LIBMATE_PANEL_APPLET_DIR@
applet_DATA		=	org.mate.panel.compa.mate-panel-applet


gsettings_SCHEMAS	=	org.mate.panel.applet.compa.gschema.xml
@GSETTINGS_RULES@


CLEANFILES		=	$(service_DATA)				\
				$(applet_DATA)				\
				$(gsettings_SCHEMAS)
