# Path to top of source tree
srcdir=..

# Installation directories
prefix=/usr/local
includedir=$(prefix)/include
dataroodir=$(prefix)/share
docdir=$(datarootdir)/doc
mandir=$(datarootdir)/man

.PHONY: clean all realclean test

demos:=demo_basic
tests:=
public_includes:=linear_ptr.h

all: $(demos)

CXXFLAGS+=-std=c++11 -g -fopenmp -O3
ifeq ($(CXX),clang++)
CXXFLAGS+=-stdlib=libc++
endif

CPPFLAGS+=-I$(srcdir)

# directories

vpath %.h $(srcdir)
vpath demo% $(srcdir)/demo

# build demos
demo_basic: demo_basic.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# clean up

realclean:
	rm -f $(demos)

