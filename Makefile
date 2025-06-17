SOURCE  := $(wildcard src/*.c) 
OBJS    := $(patsubst %.c,%.o,$(SOURCE))
  
#target you can change test to what you want
TARGET  := cproxy
  
#compile and lib parameter
CC      := gcc
LIBS    :=
LDFLAGS :=
DEFINES :=
INCLUDE := -I./include
CFLAGS  := -g -Wall -O0 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

.PHONY: all objs clean distclean rebuild
  
all: $(TARGET)
  
objs: $(OBJS)
  
rebuild: distclean 
                
clean:
	rm -fr src/*.o
    
distclean: clean
	rm -fr $(TARGET)
  
$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)