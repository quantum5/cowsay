CXX?=g++
ifdef OLDGCC
CXX+= -std=c++0x
else
CXX+= -std=c++11
endif
ifdef DEBUG
CXXFLAGS=-O0
else
CXXFLAGS=-O3
endif
ifdef WIN32
CXXFLAGS+=-DWIN32
LDFLAGS=-lshlwapi
SUFFIX=.exe
else
LDFLAGS=-lboost_regex
endif
CXXFLAGS+=$(INCPATH) $(LIBPATH)

all: cowsay$(SUFFIX)

cowsay$(SUFFIX): cowsay.o OptionParser.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

cowsay.cpp: OptionParser.h
OptionParser.cpp: OptionParser.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<