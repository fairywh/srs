CC  = gcc 
CXX = g++ 



CFLAGS +=-DCUMULUS_LOGS  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -fPIC -g -O3 -fno-strict-aliasing -Wall
INCLUDE +=  -I/usr/local/include/   -I./src/

LIBDIR = -L../../lib/ -ldl


LIB += -lpthread

LDFLAGS = $(CFLAGS) $(INCLUDE) $(LIBDIR) $(LIB)

CPP_SRC = $(wildcard */*.cpp)
OA = $(patsubst %.cpp, %.o,$(CPP_SRC))

C_SRC = $(wildcard */*.c)
OB = $(patsubst %.c, %.o,$(C_SRC))


TARGETS = ./exampled

all: $(TARGETS) install


	
$(TARGETS):
	
	@make -C src all
	@make -C example all
	$(CXX) $(CFLAGS)   ${OB} $(OA)  -o $@ $(LDFLAGS)   



clean:
	rm -f ${OA}
	rm -f ${OB}
	rm -f $(OC)
	rm -f $(TARGETS)

install:
	

