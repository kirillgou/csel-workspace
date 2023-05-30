##CXX?=g++
##CXXFLAGS=-Wall -Wextra -g -O0 -MD

TOOLCHAIN_PATH=/buildroot/output/host/usr/bin/
TOOLCHAIN=$(TOOLCHAIN_PATH)aarch64-linux-
CXX=$(TOOLCHAIN)g++
CXXFLAGS=-Wall -Wextra -g -gdwarf -O0 -MD -mcpu=cortex-a53 -fno-omit-frame-pointer -funwind-tables

SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=read-apache-logs

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

.c.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@


clean:
	@rm -f $(OBJECTS)
	@rm -f *.d *~

clean_all: clean
	@rm -f $(EXECUTABLE)
	@rm -f perf.data perf.data.old

-include *.d
