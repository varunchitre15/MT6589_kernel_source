
#PIN_ROOT is taken from env var.
PIN_BIN=${PIN_ROOT}/pin

INLINE=
OPTLEVEL=-O3
CXXFLAGS_L=$(OPTLEVEL) $(INLINE) -Wall -Werror -Wno-unknown-pragmas -g -fno-omit-frame-pointer -fno-strict-aliasing -fPIC
CXXFLAGS_L64=-DBIGARRAY_MULTIPLIER=1 -DUSING_XED  -DTARGET_IA32E -DHOST_IA32E  -DTARGET_LINUX
CXXFLAGS_L32=-DBIGARRAY_MULTIPLIER=1 -DUSING_XED  -DTARGET_IA32  -DHOST_IA32   -DTARGET_LINUX
INCLUDES_L64=-I..  -I$(PIN_ROOT)/extras/xed2-intel64/include -I$(PIN_ROOT)/source/include -I$(PIN_ROOT)/source/include/gen
INCLUDES_L32=-I..  -I$(PIN_ROOT)/extras/xed2-ia32/include -I$(PIN_ROOT)/source/include -I$(PIN_ROOT)/source/include/gen
LIBPATHS_L64=-L$(PIN_ROOT)/extras/xed2-intel64/lib -L$(PIN_ROOT)/intel64/lib -L$(PIN_ROOT)/intel64/lib-ext
LIBPATHS_L32=-L$(PIN_ROOT)/extras/xed2-ia32/lib -L$(PIN_ROOT)/ia32/lib -L$(PIN_ROOT)/ia32/lib-ext
LDFLAGS_L=-g -shared -Wl,-Bsymbolic -Wl,--version-script=$(PIN_ROOT)/source/include/pintool.ver
LIBS_L= -lpin  -lxed -ldwarf -lelf -ldl -lpthread


ifeq ($(OS), l)
  SO=so
  OBJ=o
  CXX=g++
  LD=g++
  LIBS=$(LIBS_L)
  ifeq ($(B), 64)
    CXXFLAGS=$(CXXFLAGS_L) $(CXXFLAGS_L64) $(INCLUDES_L64) -m64
    LDFLAGS=$(LDFLAGS_L) $(LIBPATHS_L64) -m64
  else
    CXXFLAGS=$(CXXFLAGS_L) $(CXXFLAGS_L32) $(INCLUDES_L32) -m32
    LDFLAGS=$(LDFLAGS_L) $(LIBPATHS_L32) -m32
  endif
else ifeq ($(OS), w)
  SO=dll
  OBJ=obj
  CXX=cl
  LD=link
  CXXFLAGS=/c /MT /EHs- /EHa- /wd4530  /DTARGET_WINDOWS /DBIGARRAY_MULTIPLIER=1 /DUSING_XED /D_CRT_SECURE_NO_DEPRECATE /D_SECURE_SCL=0 /nologo /Gy /O2 /DTARGET_IA32 /DHOST_IA32 \
	   /I.. /I$(PIN_ROOT)/source/include /I$(PIN_ROOT)/source/include/gen   /I$(PIN_ROOT)/extras/xed2-ia32/include
  LDFLAGS=/DLL /EXPORT:main /NODEFAULTLIB  /NOLOGO /INCREMENTAL:NO  /OPT:REF  /MACHINE:x86 /ENTRY:Ptrace_DllMainCRTStartup@12 /BASE:0x55000000    \
	  /LIBPATH:$(PIN_ROOT)/ia32/lib /LIBPATH:$(PIN_ROOT)/ia32/lib-ext  /LIBPATH:$(PIN_ROOT)/extras/xed2-ia32/lib
  LIBS=pin.lib libxed.lib libcpmt.lib libcmt.lib pinvm.lib kernel32.lib ntdll-32.lib
else

endif

ifeq ($(DEBUG), 1)
  BUILD_SUFIX=_deb
  DEFINES=-DDEBUG=1
else
  BUILD_SUFIX=_opt
  DEFINES=-DINCLUDE_THREAD_SANITIZER_CC=1
endif


SUFIX=_${OS}$(B)$(BUILD_SUFIX)

ALL_OBJECTS=ts_pin$(SUFIX).$(OBJ) ts_util$(SUFIX).$(OBJ) thread_sanitizer$(SUFIX).$(OBJ)

all:

pintool: ts_pin$(SUFIX).$(SO)

l: l32 l64
l64: l64o l64d
l32: l32o l32d
ld: l64d l32d
lo: l64o l32o

l64d:
	$(MAKE) pintool OS=l B=64  DEBUG=1
l64o:
	$(MAKE) pintool OS=l B=64  DEBUG=0

l32d:
	$(MAKE) pintool OS=l B=32  DEBUG=1
l32o:
	$(MAKE) pintool OS=l B=32  DEBUG=0

w32d:
	$(MAKE) pintool OS=w B=32  DEBUG=1




ts_pin$(SUFIX).so: $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) $(LIBPATHS) -o $@ $^  $(LIBS)

ts_pin$(SUFIX).dll: $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) $(LIBPATHS)  /IMPLIB:ts_pin$(SUFIX).lib /PDB:ts_pin$(SUFIX).pdb /OUT:$@  $^  $(LIBS)

HEADERS=../thread_sanitizer.h ../ts_util.h


%$(SUFIX).o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ -c $< $(DEFINES)
%$(SUFIX).o: ../%.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ -c $< $(DEFINES)

%$(SUFIX).obj: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) /Fo$@ -c $< $(DEFINES)
%$(SUFIX).obj: ../%.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) /Fo$@ -c $< $(DEFINES)


#all_tests: simple_pin_test.so detach_example.so
#%.so: %.$(OBJ)
#	$(CXX) $(LDFLAGS) $(LIBPATHS) -o $@ $<  $(LIBS)
#
#
win_test.exe: win_test.cpp
	cl /Zi $<


clean:
	rm -fv *.so *.o *.obj *.dll core* pintool.log* pin.log *.exp *.lib *.pdb *.ilk  *.exe
