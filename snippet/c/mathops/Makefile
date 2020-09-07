.PHONY: clean build

PWD:=$(shell pwd)

BUILD_DIR=$(PWD)
LIB_DIR=$(PWD)
OBJ_DIR=$(PWD)
BIN_DIR=$(PWD)
SRC_DIR=$(PWD)
UTIL_DIR=$(PWD)

CC:=gcc
CXX:=g++
CXXFLAGS:=-Wall -O3 -march=native -mtune=native
# CXXFLAGS:=-std=gnu++11 -march=skylake-avx512 -O3
# CXXFLAGS:=-std=gnu++11 -mavx2 -mfma -O3


LFLAGS:= -lrt

SRCS= $(wildcard $(SRC_DIR)/*.cpp)

OBJS=$(SRCS:%.cpp=%.o)

TARGET_EXE=vec_avx

# all: clean build post_clean

all: build

build: $(TARGET_EXE)
$(TARGET_EXE):$(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LFLAGS) -o $@

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
clean:
	find $(SRC_DIR)/ -name "*.o" | xargs rm -rf
	find $(UTIL_DIR)/ -name "*.o" | xargs rm -rf
	find $(BIN_DIR)/ -name "$(TARGET_EXE)" | xargs rm -rf

post_clean:
	find $(SRC_DIR)/ -name "*.o" | xargs rm -rf
	find $(UTIL_DIR)/ -name "*.o" | xargs rm -rf
t
