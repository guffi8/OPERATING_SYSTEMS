CC=g++

SRC=srftp.cpp clftp.cpp ftputil.cpp
HEADERSRC=ftputil.h
OBJ=$(SRC:.cpp=.o)
CXXFLAGS = -Wall -std=c++11 
TARGETS=clftp srftp

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=Makefile README performance.jpg $(SRC) $(HEADERSRC)

all: $(TARGETS)

clftp: clftp.o ftputil.o
	$(CXX) $^ -o $@ -lpthread

srftp: srftp.o ftputil.o
	$(CXX) $^ -o $@ -lpthread

clean:
	$(RM) $(TARGETS) $(OBJ) *~ *core
	
%.o: %.cpp %.h
	$(CXX) -c $(CXXFLAGS) $< -o $@

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: all clean
