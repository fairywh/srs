CC  = gcc 
CXX = g++ 



#CFLAGS  = -g3  -Wall -D_GNU_SOURCE -fPIC -shared -m64 -DSUS_LINUX -DNDEBUG
CFLAGS +=-DCUMULUS_LOGS  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -fPIC -g -O3 -fno-strict-aliasing #-Wall
INCLUDE += -I/usr/local/include/

LIB=../lib/cirmem.a
LDFLAGS = $(CFLAGS) $(INCLUDE) 

C_SRC = $(wildcard *.c)
C_OBJS = $(patsubst %.c, %.o,$(C_SRC))

CPP_SRC = $(wildcard *.cpp)
CPP_OBJS = $(patsubst %.cpp, %.o,$(CPP_SRC))

all: $(LIB)

$(LIB) : $(CPP_OBJS)  $(C_OBJS)
	rm -f $@
	ar rc $@ $(CPP_OBJS)  $(C_OBJS)
	rm -f $(CPP_OBJS)  $(C_OBJS)
    
.c.o:
	$(CC)  $(CFLAGS) -c  $(INCLUDE) $< 
	echo $@

.cpp.o:
	$(CXX)  $(CFLAGS) -c  $(INCLUDE) $< 
	echo $@
	
clean:
	rm -f *.o
	