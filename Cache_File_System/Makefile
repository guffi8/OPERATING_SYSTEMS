CC=g++
RANLIB=ranlib

LIBSRC=CachingFileSystem.cpp CacheFileSystemLog.cpp CacheFileSystemBuffer.cpp
LIB_HEADERS=CacheFileSystemLog.h CacheFileSystemBuffer.h
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
FUSE=`pkg-config fuse --cflags`
CFLAGS = -std=c++11 -Wall -g  $(INCS)

OUTPUT = CachingFileSystem
TARGETS = $(OUTPUT)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS=$(LIBSRC) $(LIB_HEADERS)  Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(CC) $^ `pkg-config fuse --libs` -o $@

clean:
	$(RM) $(TARGETS) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INC) $(FUSE) $< -o $@

CachingFileSystem.o: CachingFileSystem.cpp
	$(CC) -c $(CFLAGS) $(FUSE) $(INC) $< -o $@

tar:
	$(TAR) $(TARFLAGS)  $(TARNAME) $(TARSRCS)

force:
	touch $(LIBSRC)
	make
