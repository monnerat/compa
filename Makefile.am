## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS	=	foreign

SUBDIRS			=	src lang

ACLOCAL_AMFLAGS		=	-I m4

PANDOC			=	pandoc

dist_doc_DATA		=	COPYING					\
				AUTHORS					\
				README					\
				README.md				\
				screenshot.png


iconsdir		=	${datadir}/pixmaps
icon			=	compa.png
dist_icons_DATA		=	$(icon)

pkgdatadir		=	@PKGDATADIR@
pkgdata_DATA		=	$(icon)

EXTRA_DIST		=	compa.spec


README:	README.md
	if which "$(PANDOC)" 2>&-;					\
	then	sed -e '/<pandoc-disable>/,/<pandoc-enable>/d'		\
		    -e 's/\[\([^]]*\)]\(([^)]*)\)/\1 \2/g'		\
		    -e 's/<!-- *\(<pandoc-header>\) *-->/\\\1/g'	\
		    $<						      |	\
		"$(PANDOC)" -t plain --eol lf --columns 80	      |	\
		sed -e '/<pandoc-header>/{s///g;s/.*/\U&/;1!s/./\n&/}'	\
		    > $@;						\
	fi
