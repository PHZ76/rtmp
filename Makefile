DEUBG = -DXOP_DEBUG

TARGET1 = rtmp_server
TARGET2 = 
TARGET3 = 

OBJS_PATH = objs

CROSS_COMPILE =
CXX   = $(CROSS_COMPILE)g++
CC    = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

INC  = -I$(shell pwd)/src/ 
INC += -I$(shell pwd)/src/net -I$(shell pwd)/src/xop
INC += -I$(shell pwd)/src/3rdpart
LIB  = 

LD_FLAGS  = -lrt -pthread -lpthread -ldl -lm $(DEBUG)
CXX_FLAGS = -std=c++11

SRC1  = $(notdir $(wildcard ./src/net/*.cpp))
OBJS1 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC1))

SRC2  = $(notdir $(wildcard ./src/xop/*.cpp))
OBJS2 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC2))

SRC3  = $(notdir $(wildcard ./example/rtmp_server.cpp))
OBJS3 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC3))

SRC4  = $(notdir $(wildcard ./src/3rdpart/mongoose/*.c))
OBJS4 = $(patsubst %.c,$(OBJS_PATH)/%.o,$(SRC4))

all: BUILD_DIR $(TARGET1)

BUILD_DIR:
	@-mkdir -p $(OBJS_PATH)

$(TARGET1) : $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4)
	$(CXX) $^ -o $@ $(CFLAGS) $(LD_FLAGS) $(CXX_FLAGS)
    
$(OBJS_PATH)/%.o : ./example/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./src/net/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./src/xop/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./src/3rdpart/mongoose/%.c
	$(CC) -c  $< -o  $@ $(INC)
	
clean:
	-rm -rf $(OBJS_PATH) $(TARGET1)
