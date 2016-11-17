.PHONY: clean build

PWD:=$(shell pwd)

BUILD_DIR=$(PWD)
LIB_DIR=$(PWD)/../../lib
OBJ_DIR=$(PWD)
BIN_DIR=$(PWD)/../../bin
SRC_DIR=$(PWD)

CC:=gcc
CXX:=g++
CXXFLAGS:=-Wall -g -O

LFLAGS:= -L$(LIB_DIR) -laudiorecog -lopencv_core -lopencv_ml -lippcore -lpthread -lstdc++ -lrt

CXXFLAGS+=-I$(PWD)/../../include -I$(PWD)/../../utils

SRCS= $(wildcard $(SRC_DIR)/*.cpp)

#PUREFILE=$(notdir $(SRCS))
#PUREOBJ=$(patsubst %.cpp,%.o,$(PUREFILE) )
#OBJS=$(addprefix $(BUILD_DIR)/, $(PUREOBJ))
OBJS=$(SRCS:%.cpp=%.o)

TARGET_EXE=ModelTrain

all: clean build post_clean

build: $(TARGET_EXE)
$(TARGET_EXE):$(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LFLAGS) -o $@
	mv $(TARGET_EXE) $(BIN_DIR)/

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	find $(SRC_DIR)/ -name "*.o" | xargs rm -rf
	find $(BIN_DIR)/ -name "$(TARGET_EXE)" | xargs rm -rf

post_clean:
	find $(SRC_DIR)/ -name "*.o" | xargs rm -rf
