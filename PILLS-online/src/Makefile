# 简单 Makefile，仅编译 RAP.cpp
TARGET = RAP

CXX = g++
CXXFLAGS = -std=c++17 -O2 
INCLUDES = -I./sls_solver
SRCDIR = .
SOLVERDIR = ./sls_solver


# 只编译 main.cpp 和 file_reader.cpp
CXX = g++
CXXFLAGS = -std=c++17 -O2
SRCS = main.cpp file_reader.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
