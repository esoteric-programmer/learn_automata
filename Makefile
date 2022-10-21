appname := automata
CXX := g++
CXXFLAGS := -Wall -g --std=c++11
DEPS := $(shell find . -maxdepth 1 -name "*.h")
LDLIBS :=

srcfiles := $(shell find . -maxdepth 1 -name "*.cpp")
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname)

.PHONY: clean
.PHONY: dist-clean

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS)

%.o: %.cpp $(DEPS)
	$(CXX)  -c $< $(CXXFLAGS) 

clean:
	rm -f $(objects)

dist-clean: clean
	rm -f *~
