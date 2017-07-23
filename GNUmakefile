CXX?=g++

GCC_HAS_REGEX := $(shell expr `gcc -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/'` \>= 40900)

ifeq "$(GCC_HAS_REGEX)" "0"
  LDFLAGS:=-lboost_regex
endif

ifdef OLDGCC
  CZZ := $(CXX) -std=c++0x
else
  CZZ := $(CXX) -std=c++11
endif

ifdef DEBUG
  CXXFLAGS:=-O0
else
  CXXFLAGS:=-O3
  LDFLAGS+=-s
endif

ifdef WIN32
  CXXFLAGS+=-DWIN32
  LDFLAGS+=-lshlwapi
  SUFFIX=.exe
endif

ifdef STATIC
  LDFLAGS+=-static
endif

CXXFLAGS+=$(INCPATH) $(LIBPATH)

all: cowsay$(SUFFIX)

cowsay$(SUFFIX): cowsay.o OptionParser.o
	$(CZZ) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

cowsay.cpp: OptionParser.h
OptionParser.cpp: OptionParser.h

%.o: %.cpp
	$(CZZ) $(CXXFLAGS) -c $<
