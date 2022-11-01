## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS	=	foreign

SUBDIRS			=	src lang

ACLOCAL_AMFLAGS		=	-I m4

dist_doc_DATA		=	COPYING					\
				AUTHORS					\
				README					\
				example.html				\
				screenshot.png


iconsdir		=	${datadir}/pixmaps
icon			=	compa.png
dist_icons_DATA		=	$(icon)

pkgdatadir		=	@PKGDATADIR@
pkgdata_DATA		=	$(icon)

EXTRA_DIST		=	compa.spec
