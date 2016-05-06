.PHONY: clean build

PWD:=$(shell pwd)

BUILD_DIR=$(PWD)
LIB_DIR=$(PWD)/../../lib
OBJ_DIR=$(PWD)
SRC_DIR=$(PWD)/../../src
UTIL_DIR=$(PWD)/../../utils

CC:=gcc
CXX:=g++
CXXFLAGS:=-Wall -g -O3
LFLAGS:=-lpthread

CXXFLAGS+=-I$(PWD)/../../include -I$(PWD)/../../utils -I$(PWD)/../../src/feature -I/usr/local/include/opencv
CXXFLAGS+=-DIPP

SRCS=$(wildcard $(UTIL_DIR)/math/*.cpp) \
	$(wildcard $(UTIL_DIR)/io/file/*.cpp) \
	$(wildcard $(UTIL_DIR)/misc/*.cpp) \
	$(wildcard $(UTIL_DIR)/type/*.cpp) \
	$(wildcard $(SRC_DIR)/frontend/vad/*.cpp) \
	$(wildcard $(SRC_DIR)/frontend/denoise/*.cpp) \
	$(wildcard $(SRC_DIR)/frontend/preprocess/*.cpp) \
	$(wildcard $(SRC_DIR)/frontend/window/*.cpp) \
	$(wildcard $(SRC_DIR)/frontend/feature/*.cpp) \
	$(wildcard $(SRC_DIR)/match/*.cpp) \
	$(wildcard $(SRC_DIR)/ml/cluster/*.cpp) \
	$(wildcard $(SRC_DIR)/ml/pca/*.cpp) \
	$(wildcard $(SRC_DIR)/ml/gmm/*.cpp) \
	$(wildcard $(SRC_DIR)/ml/hmm/*.cpp) \

#PUREFILE=$(notdir $(SRCS))
#PUREOBJ=$(patsubst %.cpp,%.o,$(PUREFILE) )
#OBJS=$(addprefix $(BUILD_DIR)/, $(PUREOBJ))
OBJS=$(SRCS:%.cpp=%.o)

TARGET_STATIC=libaudiorecog.a

all: clean build post_clean

build: $(TARGET_STATIC)
$(TARGET_STATIC):$(OBJS)
	ar crv $(TARGET_STATIC) $(OBJS)
	ranlib $(TARGET_STATIC)
	mv $(TARGET_STATIC) $(LIB_DIR)/ 

#$(OBJS):$(SRCS)
#	$(CXX) $(CXXFLAGS) -o $@ -c $(SRCS)

%.o:%.cpp
	#mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	find $(LIB_DIR)/ -name *.a | xargs rm -rf
	find $(SRC_DIR)/ -name *.o  | xargs rm -rf
	find $(UTIL_DIR)/ -name *.o  | xargs rm -rf

post_clean:
	find $(SRC_DIR)/ -name *.o  | xargs rm -rf
	find $(UTIL_DIR)/ -name *.o  | xargs rm -rf
