
MAINTAINERCLEANFILES = Makefile.in

EDJE_CC = ${edje_cc}

pluginsdir = $(libdir)/ethumb/plugins
filesdir = $(pluginsdir)/data
files_DATA = emotion_template.edj

EXTRA_DIST= \
      template.edc

AM_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/plugins/emotion \
-DPLUGINSDIR=\"$(pluginsdir)\" \
@ECORE_EVAS_CFLAGS@ \
@EET_CFLAGS@ \
@EFL_ETHUMB_BUILD@ \
@EVAS_CFLAGS@ \
@EINA_CFLAGS@ \
@EMOTION_CFLAGS@ \
@ECORE_FILE_CFLAGS@ \
@EDJE_CFLAGS@

pkgdir = $(pluginsdir)
pkg_LTLIBRARIES = emotion.la
emotion_la_SOURCES = emotion.c
emotion_la_LIBADD = \
$(top_builddir)/src/lib/libethumb.la \
@ECORE_EVAS_LIBS@ \
@EET_LIBS@ \
@EMOTION_LIBS@ \
@EDJE_LIBS@ \
@ECORE_FILE_LIBS@ \
@EVAS_LIBS@ \
@EINA_LIBS@
emotion_la_LDFLAGS = -no-undefined -module -avoid-version
emotion_la_LIBTOOLFLAGS = --tag=disable-static


emotion_template.edj: Makefile $(EXTRADIST)
	$(EDJE_CC) \
		$(top_srcdir)/src/plugins/emotion/template.edc \
		$(top_builddir)/src/plugins/emotion/emotion_template.edj

clean-local:
	rm -f *.edj
