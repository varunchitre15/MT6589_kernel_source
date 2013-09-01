SVN_ROOT=..
OPT=1

include ../common.mk

OUTDIR=bin
VALGRIND_ROOT=../third_party/valgrind
VALGRIND_INST_ROOT=../tsan_inst
STLPORT_ROOT=../third_party/stlport
GTEST_ROOT=../third_party/googletest
OFFLINE=1

TS_VERSION=$(shell svnversion)

COPT=-O -fno-omit-frame-pointer
CWARN=-Wall -Wmissing-prototypes -Wshadow -Wpointer-arith -Wstrict-prototypes \
      -Wmissing-declarations -Wno-format-zero-length \
      -fno-strict-aliasing -Wno-long-long -Wno-pointer-sign \
      -Wdeclaration-after-statement
CFLAGS=-g $(COPT) $(CWARN) -fno-strict-aliasing -fno-stack-protector -fpic
CXXOPT=-O2
CXXFLAGS=$(CXXOPT) -g -Wall -Wno-deprecated -fno-exceptions # -Wvla
LDFLAGS=

OFFLINE_DEFINES=-DTS_OFFLINE=1

VG_CXXFLAGS=-fno-rtti -fno-stack-protector
VG_DEFINES=-DVGA_$(ARCH)=1 -DVGO_$(OS)=1 -DVGP_$(ARCH_OS)=1 -D_STLP_NO_IOSTREAMS=1 -DTS_VALGRIND=1
VG_INCLUDES=-I$(VALGRIND_ROOT) -I$(VALGRIND_ROOT)/include -I$(VALGRIND_ROOT)/VEX/pub -I$(STLPORT_ROOT)

PIN_CXXFLAGS_L=-fPIC
PIN_DEFINES=-DBIGARRAY_MULTIPLIER=1 -DUSING_XED -DTARGET_$(PIN_ARCH2)  -DHOST_$(PIN_ARCH2)   -DTARGET_LINUX -DTS_PIN=1
PIN_INCLUDES=-I$(PIN_ROOT)/source/include -I$(PIN_ROOT)/source/include/gen \
             -I$(PIN_ROOT)/extras/xed2-$(PIN_ARCH)/include -I$(PIN_ROOT)/extras/components/include
PIN_LIBPATHS=-L$(PIN_ROOT)/extras/xed2-$(PIN_ARCH)/lib -L$(PIN_ROOT)/$(PIN_ARCH)/lib -L$(PIN_ROOT)/$(PIN_ARCH)/lib-ext
PIN_LDFLAGS=-g -shared -Wl,-Bsymbolic -Wl,--version-script=$(PIN_ROOT)/source/include/pintool.ver
PIN_LIBS= -lpin -lxed -ldwarf -lelf -ldl -lpthread

DR_CXXFLAGS=-fPIC -fno-rtti -fno-stack-protector
DR_DEFINES=-D$(DR_OS) -D$(DR_ARCH)
DR_INCLUDES=-I$(DYNAMORIO_ROOT)/include
DR_LDFLAGS=-shared -nostartfiles -nodefaultlibs -lgcc
DR_LIBS=

OS=   # linux, darwin
ARCH= # x86, amd64
D=    # <empty>, -debug
ARCHOS=$(ARCH)-$(OS)
ARCH_OS=$(ARCH)_$(OS)

# prefix of binary files
P=$(OUTDIR)/$(ARCHOS)$(D)-
OFF=$(P)off-
VGP=$(P)vg-
PINP=$(P)pin-
PINMTP=$(P)pinmp-
DRP=$(P)dr-

STRIP=strip

INCLUDES=-I../dynamic_annotations
DEFINES=-DTS_VERSION=\"$(TS_VERSION)\"

ifeq ($(DEBUG), 1)
  D=-debug
  DEFINES+=-DDEBUG=1
else
  D=
endif

ifeq ($(ARCH), x86)
  PIN_ARCH=ia32
  PIN_ARCH2=IA32
  DR_ARCH=X86_32
else ifeq ($(ARCH), amd64)
  PIN_ARCH=intel64
  PIN_ARCH2=IA32E
  DR_ARCH=X86_64
  DR_LD_SCRIPT=-Xlinker -T -Xlinker ./dr64_ldscript # TODO(kcc) this is too hackish...
endif

ifeq ($(OS), linux)
  PIN_CXXFLAGS=$(PIN_CXXFLAGS_L)
  VG_LD_FLAGS=-Wl,--build-id=none -Wl,-Ttext=0x38000000 -static -nodefaultlibs -nostartfiles -u _start 
  VG_LD_PRELOAD_FLAGS= -nodefaultlibs -shared -Wl,-z,interpose,-z,initfirst
  DR_OS=LINUX
  STRIP=strip -g  # keep some symbols for profiling.
else ifeq ($(OS), darwin)
ifeq ($(ARCH), x86)
  VG_LD_FLAGS=-Wl,-u,__start -Wl,-e,__start -Wl,-bind_at_load /usr/lib/dyld -arch i386 \
      -Wl,-seg1addr,0xf0080000 -Wl,-stack_addr,0xf0080000 -Wl,-stack_size,0x80000 -Wl,-pagezero_size,0xf0000000 \
      -nodefaultlibs -nostartfiles
  VG_LD_PRELOAD_FLAGS=-arch i386 -dynamic -dynamiclib -all_load
endif
ifeq ($(ARCH), amd64)
  VG_LD_FLAGS=-Wl,-u,__start -Wl,-e,__start -Wl,-bind_at_load /usr/lib/dyld -arch x86_64 \
      -Wl,-seg1addr,0xf0080000 -Wl,-stack_addr,0xf0080000 -Wl,-stack_size,0x80000 -Wl,-pagezero_size,0xf0000000 \
      -nodefaultlibs -nostartfiles
  VG_LD_PRELOAD_FLAGS=-arch x86_64 -dynamic -dynamiclib -all_load
endif
  VG_CXXFLAGS+=-fno-weak -bind_at_load -fno-PIC
else ifeq ($(OS), windows)
  VALGRIND_ROOT= # no valgrind on windows :(
  CXXFLAGS=/c /Zi /MT /EHs- /EHa- /wd4530 /D_CRT_SECURE_NO_DEPRECATE /D_SECURE_SCL=0 /D_HAS_ITERATOR_DEBUGGING=0 /nologo /Gy /Ox /GL
  PIN_DEFINES=/DTARGET_WINDOWS /DBIGARRAY_MULTIPLIER=1 /DUSING_XED /DTARGET_IA32 /DHOST_IA32a /D_SECURE_SCL=0 -DTS_PIN=1
  PIN_INCLUDES=/I$(PIN_ROOT)/source/include /I$(PIN_ROOT)/source/include/gen /I$(PIN_ROOT)/extras/xed2-ia32/include
  PIN_LDFLAGS=/LTCG /DEBUG /DLL /EXPORT:main /NODEFAULTLIB /NOLOGO /INCREMENTAL:NO /OPT:REF /MACHINE:x86 /ENTRY:Ptrace_DllMainCRTStartup@12 /BASE:0x55000000
  LDFLAGS=/LTCG
  PIN_LIBPATHS= /LIBPATH:$(PIN_ROOT)/ia32/lib /LIBPATH:$(PIN_ROOT)/ia32/lib-ext /LIBPATH:$(PIN_ROOT)/extras/xed2-ia32/lib
  PIN_LIBS=pin.lib libxed.lib libcpmt.lib libcmt.lib pinvm.lib kernel32.lib ntdll-32.lib winmm.lib
  DR_OS=WINDOWS
else
  OS=UNKNOWN_OS
endif



VALGRIND_LIBS=$(VALGRIND_ROOT)/coregrind/libcoregrind-$(ARCHOS).a \
		  $(VALGRIND_ROOT)/VEX/libvex-$(ARCHOS).a

all: TS_valgrind TS_pin TS_offline TS_dynamorio test

l: l32 l64
lo: l32o l64o
ld: l32d l64d
l32: l32d l32o
l64: l64d l64o


l64d:
	$(MAKE) all OS=linux ARCH=amd64 DEBUG=1
l64o:
	$(MAKE) all OS=linux ARCH=amd64 DEBUG=0
l32d:
	$(MAKE) all OS=linux ARCH=x86 DEBUG=1
l32o:
	$(MAKE) all OS=linux ARCH=x86 DEBUG=0
la:
	$(MAKE) all OS=linux ARCH=arm DEBUG=0

w: w32
w32: w32o w32d

w32d:
	$(MAKE) all OS=windows ARCH=x86 DEBUG=1

w32o:
	$(MAKE) all OS=windows ARCH=x86 DEBUG=0

m: m32 m64
m32: m32o m32d
m64: m64o m64d

m32o:
	$(MAKE) all OS=darwin ARCH=x86 DEBUG=0
m32d:
	$(MAKE) all OS=darwin ARCH=x86 DEBUG=1
m64o:
	$(MAKE) all OS=darwin ARCH=amd64 DEBUG=0
m64d:
	$(MAKE) all OS=darwin ARCH=amd64 DEBUG=1

install:
	@echo The 'install' target is deprecated.

self-contained: TS_valgrind
	./mk-self-contained-valgrind.sh $(VALGRIND_INST_ROOT) tsan$(D) $(OUTDIR)/tsan-$(ARCHOS)$(D)-self-contained.sh
self-contained-stripped: TS_valgrind
	$(STRIP) $(OUTDIR)/tsan-*$(OS)
	./mk-self-contained-valgrind.sh $(VALGRIND_INST_ROOT) tsan$(D) $(OUTDIR)/tsan-$(ARCHOS)$(D)-self-contained.sh

TSAN_SFX_DIR=tsan-$(ARCHOS)
sfx:
	rm -rf $(TSAN_SFX_DIR)
	mkdir -p $(TSAN_SFX_DIR)
	cp -r $(PIN_ROOT)/ia32 \
			bin/$(ARCHOS)-debug-ts_pin.dll bin/$(ARCHOS)-ts_pin.dll \
			bin/$(ARCHOS)-ts_pinmt.dll \
		  tsan.bat tsan-debug.bat tsan_mt.bat $(TSAN_SFX_DIR)
	cp license_for_windows.txt $(TSAN_SFX_DIR)/LICENSE
	7z a -sfx tsan-$(ARCHOS)-sfx.exe $(TSAN_SFX_DIR)
	rm -rf $(TSAN_SFX_DIR)

w32-sfx: w32o w32d
	$(MAKE) sfx OS=windows ARCH=x86

ifeq ($(VALGRIND_ROOT), )
TS_valgrind:
	@echo VALGRIND_ROOT is not set. Not building the Valgrind-based variant.
else
TS_valgrind:  $(OUTDIR)/tsan$D-$(ARCHOS) $(OUTDIR)/vgpreload_tsan$D-$(ARCHOS).so
endif

ifeq ($(PIN_ROOT), )
TS_pin:
	@echo PIN_ROOT is not set. Not building the PIN-based variant.
else
TS_pin: $(P)ts_pinmt.$(SO) $(P)ts_pin.$(SO)
endif

ifeq ($(DYNAMORIO_ROOT), )
TS_dynamorio:
	@echo DYNAMORIO_ROOT is not set. Not building the DynamoRio-based variant.
else
TS_dynamorio: $(P)ts_dynamorio.$(SO)
endif


ifeq ($(OFFLINE), 1)
TS_offline: $(P)ts_offline$(EXE)
else
TS_offline:
endif

ifeq ($(GTEST_ROOT), )
test:
	@echo GTEST_ROOT is not set. Not building GTEST-based tests.
else
test: $(P)suppressions_test$(EXE) $(P)thread_sanitizer_test$(EXE)
endif

$(OUTDIR):
	mkdir -p $(OUTDIR)

TS_HEADERS=thread_sanitizer.h ts_util.h suppressions.h ignore.h ts_replace.h ts_heap_info.h \
	   ts_simple_cache.h ts_stats.h ts_lock.h ts_events.h ts_event_names.h \
	   ts_trace_info.h ts_race_verifier.h dense_multimap.h \
           ts_atomic.h ts_atomic_int.h \
	   ../dynamic_annotations/dynamic_annotations.h
ts_event_names.h: ts_events.h
	sed -n '/^enum/,/^};/ {s/enum EventType/static const char *kEventNames[] = /; s/^  \([A-Z_][A-Z_]*\)/  "\1"/g; p;}' $< > $@
TS_VG_HEADERS=ts_valgrind.h ts_valgrind_client_requests.h

TS_VG_OBJECTS=$(VGP)thread_sanitizer.o $(VGP)ts_valgrind.o $(VGP)ts_valgrind_libc.o $(VGP)ts_util.o $(VGP)suppressions.o $(VGP)ignore.o $(VGP)common_util.o $(VGP)ts_race_verifier.o $(VGP)ts_atomic.o
TS_PIN_OBJECTS=$(PINP)ts_pin.$(OBJ) $(PINP)ts_util.$(OBJ) $(PINP)thread_sanitizer.$(OBJ) $(PINP)suppressions.$(OBJ) $(PINP)ignore.$(OBJ) $(PINP)common_util.$(OBJ) $(PINP)ts_race_verifier.$(OBJ) $(PINP)ts_atomic.$(OBJ)
TS_PINMT_OBJECTS=$(PINMTP)ts_pin.$(OBJ) $(PINMTP)ts_util.$(OBJ) $(PINMTP)thread_sanitizer.$(OBJ) $(PINMTP)suppressions.$(OBJ) $(PINMTP)ignore.$(OBJ) $(PINMTP)common_util.$(OBJ) $(PINMTP)ts_race_verifier.$(OBJ) $(PINMTP)ts_atomic.$(OBJ)
TS_OFFLINE_OBJECTS=$(OFF)ts_offline.$(OBJ) $(OFF)thread_sanitizer.$(OBJ) $(OFF)ts_util.$(OBJ) $(OFF)suppressions.$(OBJ) $(OFF)ignore.$(OBJ) $(OFF)common_util.$(OBJ) $(OFF)ts_atomic.$(OBJ)
TS_DR_OBJECTS=$(DRP)ts_dynamorio.$(OBJ) $(DRP)ts_util.$(OBJ)

$(P)%.$(OBJ): %.cc $(TS_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(O)$@ -c $< $(DEFINES) $(INCLUDES)

$(OFF)%.$(OBJ): %.cc $(TS_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) $(OFFLINE_DEFINES) $(O)$@ -c $< $(DEFINES) $(INCLUDES)

$(VGP)%.o: %.cc $(TS_HEADERS) $(TS_VG_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(VG_CXXFLAGS) $(ARCHFLAGS) $(VG_INCLUDES) $(VG_DEFINES) -o $@ -c $< $(DEFINES) $(INCLUDES)

$(PINP)%.$(OBJ): %.cc $(TS_HEADERS) $(TS_PIN_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $(ARCHFLAGS) $(PIN_INCLUDES) $(PIN_DEFINES) $(O)$@ -c $< $(DEFINES) $(INCLUDES)

$(PINMTP)%.$(OBJ): %.cc $(TS_HEADERS) $(TS_PIN_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $(ARCHFLAGS) $(PIN_INCLUDES) $(PIN_DEFINES) $(O)$@ -c $< $(DEFINES) $(INCLUDES) -DTS_SERIALIZED=0

$(DRP)%.$(OBJ): %.cc $(TS_HEADERS) $(TS_DR_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(DR_CXXFLAGS) $(ARCHFLAGS) $(DR_INCLUDES) $(DR_DEFINES) $(O)$@ -c $< $(DEFINES) $(INCLUDES)

$(P)gtest-%.$(OBJ): %.cc $(TS_HEADERS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) -I$(GTEST_ROOT)/include $(O)$@ -c $<

$(P)preload-%.o: %.c $(TS_HEADERS) $(TS_VG_HEADERS) | $(OUTDIR)
	$(CC) $(CFLAGS) $(ARCHFLAGS) $(VG_INCLUDES) $(VG_DEFINES) -o $@ -c $<

$(OUTDIR)/tsan$(D)-$(ARCHOS): $(TS_VG_OBJECTS)
	$(LTLD) $(LDFLAGS) $(ARCHFLAGS) $(VG_LD_FLAGS) -o $@ $^ $(VALGRIND_LIBS) -lgcc
	ln -sf `pwd`/$@ $(VALGRIND_INST_ROOT)/lib/valgrind/  # install the symlink into the valgrind inst dir.

$(OUTDIR)/vgpreload_tsan$(D)-$(ARCHOS).so: $(P)preload-ts_valgrind_intercepts.o
	$(LD) $(LDFLAGS)  $(ARCHFLAGS) $(VG_LD_PRELOAD_FLAGS) -o $@  $<
	ln -sf `pwd`/$@  $(VALGRIND_INST_ROOT)/lib/valgrind/  # install the symlink into the valgrind inst dir.

$(P)ts_offline$(EXE): $(TS_OFFLINE_OBJECTS)
	$(LD) $(LDFLAGS) $(ARCHFLAGS) $(LINKO)$@ $^

$(P)suppressions_test$(EXE): $(P)gtest-suppressions_test.$(OBJ) $(P)suppressions.$(OBJ) $(P)common_util.$(OBJ) $(P)ts_util.$(OBJ) $(GTEST_LIB)
	$(LD) $(LDFLAGS) $(ARCHFLAGS) $(LINKO)$@ $^

$(P)thread_sanitizer_test$(EXE): $(P)gtest-thread_sanitizer_test.$(OBJ) $(P)ts_util.$(OBJ) $(GTEST_LIB)
	$(LD) $(LDFLAGS) $(ARCHFLAGS) $(LINKO)$@ $^

$(P)ts_pin.so: $(TS_PIN_OBJECTS)
	$(LD) $(ARCHFLAGS) $(PIN_LDFLAGS) $(PIN_LIBPATHS) -o $@ $^  $(PIN_LIBS)

$(P)ts_pinmt.so: $(TS_PINMT_OBJECTS)
	$(LD) $(ARCHFLAGS) $(PIN_LDFLAGS) $(PIN_LIBPATHS) -o $@ $^  $(PIN_LIBS)

$(P)ts_pin.dll: $(TS_PIN_OBJECTS)
	$(LD) $(ARCHFLAGS) $(PIN_LDFLAGS) $(PIN_LIBPATHS)  /IMPLIB:$(PINP)ts_pin.lib /PDB:$(PINP)ts_pin.pdb /OUT:$@  $^  $(PIN_LIBS)

$(P)ts_pinmt.dll: $(TS_PINMT_OBJECTS)
	$(LD) $(ARCHFLAGS) $(PIN_LDFLAGS) $(PIN_LIBPATHS)  /IMPLIB:$(PINMTP)ts_pin.lib /PDB:$(PINMTP)ts_pin.pdb /OUT:$@  $^  $(PIN_LIBS)

$(P)ts_dynamorio.so: $(TS_DR_OBJECTS)
	$(LD) $(ARCHFLAGS) $(DR_LDFLAGS) $(DR_LIBPATHS) -o $@ $^  $(DR_LIBS) $(DR_LD_SCRIPT)

clean: GTEST_CLEAN
	rm -rfv $(OUTDIR) ts_event_names.h core* pintool.log* pin.log $(VALGRIND_INST_ROOT)/lib/valgrind/*tsan*
