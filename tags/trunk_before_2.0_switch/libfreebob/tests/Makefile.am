# Makefile.am - Makefile template
# Copyright (C) 2005,06 by Daniel Wagner.
#
# This file is part of FreeBoB.
#
# FreeBoB is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# FreeBoB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with FreeBoB; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

SUBDIRS = streaming

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src $(LIBXML_CFLAGS) \
	$(LIBAVC1394_CFLAGS)

noinst_PROGRAMS = test-freebob test-extplugcmd test-fw410 freebob-server \
	test-volume test-mixer
noinst_HEADERS = 

test_freebob_SOURCES = test-freebob.c
test_freebob_LDADD   = $(top_builddir)/src/libfreebob.la \
	$(LIBXML_LIBS) $(LIBAVC1394_LIBS) $(LIBIEC61883_LIBS)

test_extplugcmd_SOURCES = test-extplugcmd.cpp
test_extplugcmd_LDADD = $(top_builddir)/src/libfreebob.la \
	$(LIBAVC1394_LIBS)

test_volume_SOURCES = test-volume.cpp
test_volume_LDADD = $(top_builddir)/src/libfreebob.la \
	$(LIBAVC1394_LIBS)

test_mixer_SOURCES = test-mixer.cpp
test_mixer_LDADD = $(top_builddir)/src/libfreebob.la \
	$(LIBAVC1394_LIBS)

test_fw410_SOURCES = test-fw410.cpp
test_fw410_LDADD   = $(LIBAVC1394_LIBS) $(LIBIEC61883_LIBS) -lrom1394

freebob_server_SOURCES = freebob-server.c
freebob_server_LDADD   = $(top_builddir)/src/libfreebob.la $(LIBXML_LIBS) \
	$(LIBAVC1394_LIBS) $(LIBIEC61883_LIBS) -lrom1394


#TESTS_ENVIRONMENT
TEST = test-freebob
