if WITHDOXYGEN
doc: Doxyfile all
	doxygen
endif


AM_CXXFLAGS = 
INCLUDES = -I./ 
lib_LTLIBRARIES = libapmoncpp.la
include_HEADERS = ApMon.h utils.h monitor_utils.h proc_utils.h mon_constants.h xdr.h

libapmoncpp_la_SOURCES = ApMon.cpp utils.cpp monitor_utils.cpp proc_utils.cpp mon_constants.cpp xdr.cpp

EXTRA_DIST = ApMon_win.dsp ApMon_win.dsw

libapmoncpp_la_LIBADD = -lpthread 
libapmoncpp_la_LDFLAGS = -version-info 2:6:0

SUBDIRS	= . examples 
