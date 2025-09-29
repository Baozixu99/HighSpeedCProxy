SOURCE  := $(wildcard src/*.c senario_src/*.c) 
OBJS    := $(patsubst %.c,%.o,$(SOURCE))
  
#target you can change test to what you want
TARGET  := cproxy
ARCH ?= 

#compile and lib parameter
LIBS    :=
LDFLAGS :=
DEFINES :=
INCLUDE := -I./include -I./senario_inc
CFLAGS  := -g -Wall -O0 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

ifeq ($(ARCH), arm64)
	CC := aarch64-linux-gnu-gcc
	READELF := aarch64-linux-gnu-readelf
	OBJDUMP := aarch64-linux-gnu-objdump
else
	CC := gcc
endif

.PHONY: all objs clean distclean rebuild
  
all: $(TARGET)
  
objs: $(OBJS)
  
rebuild: distclean 
                
clean:
	rm -fr src/*.o $(TARGET)
    
distclean: clean
	rm -fr $(TARGET)
  
$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
