## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS =	foreign

SUBDIRS		=	src lang

ACLOCAL_AMFLAGS	=	-I m4

doc_DATA	=	COPYING						\
			AUTHORS						\
			README						\
			example.html					\
			screenshot.png


iconsdir	=	${datadir}/pixmaps
icon		=	compa.png
icons_DATA	=	$(icon)

EXTRA_DIST	=	${doc_DATA}					\
			${icon}						\
			compa.spec
