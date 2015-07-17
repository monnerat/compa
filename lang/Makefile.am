LINGUAS			=	fr

POFILES			=	${LINGUAS:%=%.po}
MOFILES			=	${LINGUAS:%=%.mo}

LANG_C_FILES		=	../src/main.c

LANG_GSETTINGS_FILES	=	../src/org.mate.panel.applet.compa.gschema.xml
LANG_GSETTINGS_FILES_IN	=	${LANG_GSETTINGS_FILES:%=%.in}

noinst_DATA		=	$(MOFILES)

EXTRA_DIST		=	compa.pot $(POFILES)


compa.pot:	$(LANG_C_FILES) $(LANG_GSETTINGS_FILES_IN)
	> compa.pot
	rm -rf tmp
	intltool-extract -l --type=gettext/gsettings $(LANG_GSETTINGS_FILES)
	xgettext -j --language=C -k_ -kN_ --from-code=UTF-8		\
	    --package-name=compa --package-version="@VERSION@"		\
	    --copyright-holder='Patrick Monnerat'			\
	    --force-po --no-location -s -o compa.pot			\
	    $(LANG_C_FILES) tmp/*
	rm -rf tmp

install-data-hook:
	for lang in $(LINGUAS);						\
	do	mkdir -p "$(DESTDIR)$(localedir)/$${lang}/LC_MESSAGES/"; \
		cp "$${lang}.mo"					\
		    "$(DESTDIR)$(localedir)/$${lang}/LC_MESSAGES/compa.mo"; \
	done

%.po:	compa.pot
	msgmerge -s -U --backup=none --previous -q $@ compa.pot


SUFFIXES		=	.po .mo

.po.mo:
	msgfmt -o $@ $<
