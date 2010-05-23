## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gtkdbgp

gtkdbgp_SOURCES = \
	main.c \
	support.c support.h \
	callbacks.c callbacks.h \
	server.c \
	usefulstuff.c usefulstuff.h \
	xdebug_hash.c xdebug_hash.h \
	xdebug_llist.c xdebug_llist.h \
	xdebug_mm.h \
	xdebug_private.h \
	xdebug_str.c xdebug_str.h \
	xdebug_var.h \
	xdebug_xml.c xdebug_xml.h \
	xdebug_xml_reader.c

gtkdbgp_LDADD = @PACKAGE_LIBS@
gtkdbgp_LDFLAGS =  -Wl,--export-dynamic 
