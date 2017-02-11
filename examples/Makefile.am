INCLUDES = -I../
noinst_PROGRAMS = example_1 example_2 example_3 example_4 example_x1 example_x2 example_confgen example_sensor

example_1_SOURCES = example_1.cpp
example_x1_SOURCES = example_x1.cpp
example_2_SOURCES = example_2.cpp
example_x2_SOURCES = example_x2.cpp
example_3_SOURCES = example_3.cpp
example_4_SOURCES = example_4.cpp
example_confgen_SOURCES = example_confgen.cpp
example_sensor_SOURCES = example_sensor.cpp

EXTRA_DIST = destinations_1.conf destinations_3.conf destinations_x1.conf destinations_x2.conf destinations_s.conf

example_1_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_2_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_3_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_4_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_x1_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_x2_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_confgen_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
example_sensor_LDADD =  -L$(top_srcdir)/ -lpthread -lm -lapmoncpp
