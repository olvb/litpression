DEBUG = 0
TARGET = litpression

CXX = g++
LD = $(CXX)
CXXFLAGS = -std=c++14 -Wall -Wextra
LDFLAGS =

ifeq ($(DEBUG), 1)
	CXXFLAGS += -g -DDEBUG
else
	CXXFLAGS += -O2 -DNDEBUG
endif

# opencv flags
CXXFLAGS += $(shell pkg-config --cflags opencv4)
LDFLAGS += $(shell pkg-config --libs opencv4)

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp, build/obj/%.o, $(SRCS))
DEPS = $(wildcard build/deps/*.d)

.PHONY: all clean

all: build/$(TARGET)

build/$(TARGET): $(OBJS) build/obj/triangle.o
	@mkdir -p $(@D)
	$(LD) -o $@ $^ $(LDFLAGS)

build/obj/%.o: src/%.cpp
	@mkdir -p $(@D) build/deps/
	$(CXX) $(CXXFLAGS) -MMD -MF build/deps/$*.d -c -o $@ $<

build/obj/triangle.o: src/triangle.h src/triangle.c
	$(CXX) -DVOID=int -DNO_TIMER -DANSI_DECLARATORS -DTRILIBRARY src/triangle.c -c -o $@

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

clean:
	$(RM) -rf build/*
