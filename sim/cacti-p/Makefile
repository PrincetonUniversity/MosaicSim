TARGET = cacti
SHELL = /bin/sh
.PHONY: all depend clean
.SUFFIXES: .cc .o

ifndef NTHREADS
  NTHREADS = 8
endif

LIBS =
INCS = -lm

OPT = -O3 -msse2 -mfpmath=sse -DNTHREADS=$(NTHREADS)

CXXFLAGS = -Wno-unknown-pragmas $(DBG) $(OPT)
CXX = g++
CC  = gcc
AR  = ar

SRCS  = area.cc bank.cc mat.cc main.cc Ucache.cc io.cc technology.cc basic_circuit.cc parameter.cc \
		decoder.cc component.cc uca.cc subarray.cc wire.cc htree2.cc \
		cacti_interface.cc router.cc nuca.cc crossbar.cc arbiter.cc powergating.cc

OBJS = $(SRCS:.cc=.o)
PYTHONLIB_SRCS = $(patsubst main.cc, ,$(SRCS)) obj_$(TAG)/cacti_wrap.cc
PYTHONLIB_OBJS = $(patsubst %.cc,%.o,$(PYTHONLIB_SRCS))
INCLUDES       = -I /usr/include/python2.4 -I /usr/lib/python2.4/config

all: libcacti

$(TARGET): $(OBJS)
		$(CXX) $(OBJS) -o $@ $(INCS) $(CXXFLAGS) $(LIBS) -pthread

libcacti:	$(OBJS)
	$(AR) -r catci.a $(OBJS)
	cp catci.a ../../lib/

%.o: %.cc
		$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f *.o _cacti.so cacti.py catci.a $(TARGET)


