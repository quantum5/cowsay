CXX=cl /nologo

!IFDEF DEBUG
CXXFLAGS=/Od
!ELSE
CXXFLAGS=/Ox
!ENDIF

!IFNDEF NOPDB
CXXFLAGS=$(CXXFLAGS) /Zi
!ENDIF

CXXFLAGS=$(CXXFLAGS) /EHsc /DWIN32
LIBS=shlwapi.lib

cowsay.exe: cowsay.obj OptionParser.obj
	$(CXX) $(CXXFLAGS) $** $(LIBS) /Fe$@

upx: cowsay.exe
	upx --best $**

cowsay.cpp: OptionParser.h
OptionParser.cpp: OptionParser.h

.cpp.obj::
	$(CXX) $(CXXFLAGS) /c $<
