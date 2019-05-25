# Copyright 2017 Columbia University, SLD Group

USE_CBLAS = 0

#
# Main target
#

TARGET = multt 

# Directories

VPATH :=
VPATH += src
VPATH += inc

# Source files

C_SOURCES := 
C_SOURCES += multt.c
C_SOURCES += main.c

# Include files

C_HEADERS :=
C_HEADERS += multt.h

#
# Compiler options
#

# Compiler

CC = gcc

# Compiler flags

C_FLAGS :=
C_FLAGS += -O3
C_FLAGS += -Wall
# C_FLAGS += -DUSE_CBLAS

# Linker flags

# Install atlas-devel
# LD_FLAGS := -L/usr/lib64/atlas

# Linker libs

# Install atlas-devel
# LD_LIBS := -lsatlas

# Include dirs

INCDIR :=
INCDIR += -I./inc

#
# Compiler rules
#

.SUFFIXES: .c .h .o

all: $(TARGET)

C_OBJECTS := $(C_SOURCES:.c=.o)

$(C_OBJECTS): $(C_HEADERS)

$(TARGET): $(C_OBJECTS)
	@echo " [LINKING]   $(TARGET)"
	@$(CC) -o $@ $(C_OBJECTS) ${LD_LIBS} ${LD_FLAGS}
	@rm -f *.o

.c.o:
	@echo " [COMPILING] $<"
	@$(CC)$(C_FLAGS)${INCDIR} -c $<

check: $(TARGET)
	bash ./check

clean:
	@echo " [CLEANING]"
	@rm -f *.o $(TARGET)


.PHONY: all clean check
