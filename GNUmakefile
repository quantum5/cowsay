CXX?=g++
LDFLAGS=-lboost_regex

ifdef OLDGCC
  CZZ := $(CXX) -std=c++0x
else
  CZZ := $(CXX) -std=c++11
endif

ifdef DEBUG
  CXXFLAGS:=-O0
else
  CXXFLAGS:=-O3
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
