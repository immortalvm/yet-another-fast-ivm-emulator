#-----------------------------------------------------------------------
# Preservation Virtual Machine Project
# File: Makefile
#
# Authors:
#	Sergio Romero Montiel
#	Eladio Gutierrez Carrasco
#	Oscar Plata Gonzalez
#
# Created on May 2020
#-----------------------------------------------------------------------
#-----------------------FILES-------------------------------------------
EXEC_PREFIX :=ivm64-emu
EXEC_FAST	:=$(EXEC_PREFIX)-no-io
EXEC_SEQ	:=$(EXEC_PREFIX)
EXEC_PAR	:=$(EXEC_PREFIX)-parallel
EXEC_HISTO  :=$(EXEC_PREFIX)-histo
EXEC_TRACE2 :=$(EXEC_PREFIX)-trace2         #VERBOSE=2
EXEC_TRACE3 :=$(EXEC_PREFIX)-trace_compact  #VERBOSE=3
EXEC_TRACE4 :=$(EXEC_PREFIX)-trace4         #VERBOSE=4
#-----------------------TOOLS-------------------------------------------
# Compiler
CC = gcc
# Compiler options
#CFLAGS = -Wall -Ofast -I.
#CFLAGS = -Ofast -I. -DSTEPCOUNT -DFPE_ENABLED
CFLAGS = -Ofast -I. -DSTEPCOUNT
LDFLAGS = -static -lpng -lz -lm
# ----------------------RULES-------------------------------------------
# Targets y sufijos
.PHONY: all clean
#regla para hacer la libreria
all: $(EXEC_SEQ) $(EXEC_FAST) $(EXEC_PAR) $(EXEC_HISTO) $(EXEC_TRACE2) $(EXEC_TRACE3) $(EXEC_TRACE4)

$(EXEC_FAST): ivm_emu.c ivm_emu.h
	$(CC) $(CFLAGS) $< -o $@

$(EXEC_SEQ): ivm_emu.c ivm_emu.h ivm_io.h 
	$(CC) $(CFLAGS) $< -o $@ -DWITH_IO $(LDFLAGS)

$(EXEC_PAR): ivm_emu.c ivm_emu.h ivm_io.h 
	$(CC) $(CFLAGS) $< -o $@ -DWITH_IO -DPARALLEL_OUTPUT $(LDFLAGS)

$(EXEC_HISTO): ivm_emu.c ivm_emu.h ivm_io.h
	$(CC) $(CFLAGS) $< -o $@ -DWITH_IO -DSTEPCOUNT -DNOOPT -DVERBOSE=1 -DHISTOGRAM $(LDFLAGS)

$(EXEC_TRACE2): ivm_emu.c ivm_emu.h ivm_io.h
	$(CC) $(CFLAGS) $< -o $@ -DWITH_IO -DSTEPCOUNT -DNOOPT -DVERBOSE=2 $(LDFLAGS)

$(EXEC_TRACE3): ivm_emu.c ivm_emu.h ivm_io.h ivm_emu_hash_table.h
	$(CC) $(CFLAGS) $< -o $@ -DWITH_IO -DSTEPCOUNT -DNOOPT -DVERBOSE=3 $(LDFLAGS)

$(EXEC_TRACE4): ivm_emu.c ivm_emu.h ivm_io.h ivm_emu_hash_table.h
	$(CC) $(CFLAGS) $< -o $@ -DWITH_IO -DSTEPCOUNT -DNOOPT -DVERBOSE=4 $(LDFLAGS)

clean:
	-rm -fv $(EXEC_FAST) $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_HISTO) $(EXEC_TRACE2) $(EXEC_TRACE3) $(EXEC_TRACE4)
