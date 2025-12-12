SRCS = studentwork.cpp sim.cpp
OBJS = $(SRCS:.cpp=.o)

CXX = g++
CXXFLAGS = -g -Wall -Werror -pedantic -std=c++11
TARBALL = lab1.tar.gz

.PHONY: all sim clean profile debug validate fast submit

all: clean
all: sim

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

sim: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean: 
	-rm -f sim $(OBJS)

profile: clean
profile: CXXFLAGS += -O2 -pg
profile: all

debug: clean
debug: CXXFLAGS += -DDEBUG
debug: all

validate: clean
validate: all
validate: 
	@bash ../scripts/runall.sh

fast: clean
fast: CXXFLAGS += -O2
fast: all

submit:
	tar -czvf $(TARBALL) studentwork.cpp report.txt
	@echo 'Created! Please check the tarball to ensure it was made correctly!'
	@echo 'You are solely responsible for what you submit!'