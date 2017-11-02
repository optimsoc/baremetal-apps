# Copyright (c) 2012-2017 by the author(s)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# Author(s):
#   Annika Fuchs <annika.fuchs@tum.de>

PROGRAM=lwip_demo

# Build with LWIP
LWIPDIR=lwip/src
LWIP_CONTRIB_DIR=lwip-contrib/ports/optimsoc
include $(LWIPDIR)/Filelists.mk

CONTRIBFILES=$(LWIP_CONTRIB_DIR)/sys_arch.c
NETIFFILES=$(LWIPDIR)/netif/ethernet.c

EXTRA_LIBS=
EXTRA_INCS=-I $(LWIP_CONTRIB_DIR) -I $(LWIP_CONTRIB_DIR)/include  -I $(LWIPDIR)/include 
EXTRA_SRCS+=$(COREFILES) $(CORE4FILES) $(CORE6FILES) $(APIFILES) $(NETIFFILES) $(SNMPFILES) $(CONTRIBFILES)

BUILDSCRIPTS=$(shell pkg-config --variable=buildscriptdir optimsoc-baremetal-runtime)
include $(BUILDSCRIPTS)/Makefile.inc
