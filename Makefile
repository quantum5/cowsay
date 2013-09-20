CXX=cl /nologo
!IFDEF DEBUG
CXXFLAGS=/Od
!ELSE
CXXFLAGS=/Ox
!ENDIF
CXXFLAGS=$(CXXFLAGS) /Zi /EHsc /DWIN32
LIBS=shlwapi.lib

cowsay.exe: cowsay.obj OptionParser.obj
	$(CXX) $(CXXFLAGS) $** $(LIBS) /Fe$@

cowsay.cpp: OptionParser.h
OptionParser.cpp: OptionParser.h

.cpp.obj::
	$(CXX) $(CXXFLAGS) /c $<
