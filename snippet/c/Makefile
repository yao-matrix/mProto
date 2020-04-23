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
CXXFLAGS:=-Wall -g

LFLAGS:= -lrt

SRCS= $(wildcard $(SRC_DIR)/*.cpp)

OBJS=$(SRCS:%.cpp=%.o)

TARGET_EXE=cpu_checker

all: clean build post_clean

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
