# Makefile.am - Makefile template
# Copyright (C) 2005 by Daniel Wagner.
#
# This file is part of FreeBob.
#
# FreeBob is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# FreeBob is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with FreeBob; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

noinst_LTLIBRARIES = libdebugmodule.la


libdebugmodule_la_SOURCES =		\
		debugmodule.h	        \
		debugmodule.cpp       	


noinst_PROGRAMS = test_debugmodule
test_debugmodule_SOURCES = test_debugmodule.cpp
test_debugmodule_LDADD = libdebugmodule.la

MAINTAINERCLEANFILES = Makefile.in
