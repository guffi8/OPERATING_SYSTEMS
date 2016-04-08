CC=g++
RANLIB=ranlib

LIBSRC=blockchain.cpp Block.cpp BlockChainObj.cpp 
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -g $(INCS) -std=c++11
LOADLIBES = -L./ 

OSMLIB = libblockchain.a
TARGETS = $(OSMLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
TARSRCS=$(LIBSRC) Makefile README Block.h BlockChainObj.h

all: $(TARGETS)
	
$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(OSMLIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)
	
%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
