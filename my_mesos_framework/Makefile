CXX = g++
CXXFLAGS = -Wall -g -O2 -std=c++11
LDFLAGS = -lmesos -lprotobuf
CXXCOMPILE = $(CXX) $(INCLUDES) $(CXXFLAGS) -c -o $@
CXXLINK = $(CXX) $(INCLUDES) $(CXXFLAGS) -o $@

default: all
all: my_executor my_framework 

%: %.cpp $(HEADERS)
	$(CXXLINK) $< $(LDFLAGS)

clean:
	rm -f my_executor my_framework
