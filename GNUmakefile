# =========================================================================
#     This makefile was generated by
#     Bakefile 0.2.9 (http://www.bakefile.org)
#     Do not modify, all changes will be overwritten!
# =========================================================================



# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# C++ compiler 
CXX = g++

# Standard flags for C++ 
CXXFLAGS ?= 

# Standard preprocessor flags (common for CC and CXX) 
CPPFLAGS ?= -D ATIMER_USE_TICKCOUNT

# Standard linker flags 
LDFLAGS ?= 

# Location and arguments of wx-config script 
WX_CONFIG ?= wx-config

# Port of the wx library to build against [gtk1,gtk2,msw,x11,motif,mgl,osx_cocoa,osx_carbon,dfb]
WX_PORT ?= $(shell $(WX_CONFIG) --query-toolkit)

# Use DLL build of wx library to use? [0,1]
WX_SHARED ?= $(shell if test -z `$(WX_CONFIG) --query-linkage`; then echo 1; else echo 0; fi)

# Compile Unicode build of wxWidgets? [0,1]
WX_UNICODE ?= $(shell $(WX_CONFIG) --query-chartype | sed 's/unicode/1/;s/ansi/0/')

# Version of the wx library to build against. 
WX_VERSION ?= $(shell $(WX_CONFIG) --query-version | sed -e 's/\([0-9]*\)\.\([0-9]*\)/\1\2/')



# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

### Variables: ###

CPPDEPS = -MT$@ -MF$*.d -MD -MP
WX_VERSION_MAJOR = $(shell echo $(WX_VERSION) | cut -c1,1)
WX_VERSION_MINOR = $(shell echo $(WX_VERSION) | cut -c2,2)
WX_CONFIG_FLAGS = $(WX_CONFIG_UNICODE_FLAG) $(WX_CONFIG_SHARED_FLAG) \
	--toolkit=$(WX_PORT) --version=$(WX_VERSION_MAJOR).$(WX_VERSION_MINOR)
THEX_CXXFLAGS = -I. -g -I/usr/local/lib/wx/include/gtk2-unicode-2.9 -I/usr/local/include/wx-2.9 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread $(CPPFLAGS) \
	$(CXXFLAGS)
THEX_OBJECTS =  \
	atimer.o \
	BarViewCtrl.o \
	datahelp.o \
	datasource.o \
	dialogs.o \
	HexDoc.o \
	hexwnd.o \
	HexWndCmd.o \
	hwpaint.o \
	main.o \
	palette.o \
	search.o \
	settings.o \
	statusbar.o \
	struct.o \
	struct_lex.o \
	struct_par.o \
	thFrame.o \
	toolwnds.o \
	undo.o \
	utils.o \
	wxtreelistctrl.o

### Conditionally set variables: ###

ifeq ($(WX_UNICODE),0)
WX_CONFIG_UNICODE_FLAG = --unicode=no
endif
ifeq ($(WX_UNICODE),1)
WX_CONFIG_UNICODE_FLAG = --unicode=yes
endif
ifeq ($(WX_SHARED),0)
WX_CONFIG_SHARED_FLAG = --static=yes
endif
ifeq ($(WX_SHARED),1)
WX_CONFIG_SHARED_FLAG = --static=no
endif


### Targets: ###

all: test_for_selected_wxbuild thex

install: 

uninstall: 

clean: 
	rm -f ./*.o
	rm -f ./*.d
	rm -f thex

test_for_selected_wxbuild: 
	@$(WX_CONFIG) $(WX_CONFIG_FLAGS)

thex: precomp $(THEX_OBJECTS)
	$(CXX) -o $@ $(THEX_OBJECTS)   -g $(LDFLAGS)  `$(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs net,aui,core,base`

precomp: precomp.h
	$(CXX) -c -o precomp.h.gch $(THEX_CXXFLAGS) $(CPPDEPS) $<

.cpp.o:
	$(CXX) -c -o $@ $(THEX_CXXFLAGS) $(CPPDEPS) $<

wxtreelistctrl.o: wx/treelistctrl.cpp
	$(CXX) -c -o $@ $(THEX_CXXFLAGS) $(CPPDEPS) $<

.PHONY: all install uninstall clean


# Dependencies tracking:
-include ./*.d
