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
EXEC_FAST	:=ivm_emu_fast
EXEC_SEQ	:=ivm_emu_fast_io
EXEC_PAR	:=ivm_emu_fast_io_parallel
EXEC_HISTO  :=ivm_histo
EXEC_PROBE  :=ivm_probe
EXEC_TRACE  :=ivm_trace
#-----------------------TOOLS-------------------------------------------
# Compiler
CC = gcc
# Compiler options
#CFLAGS = -Wall -Ofast -I.
CFLAGS = -Ofast -I. -DSTEPCOUNT
# ----------------------RULES-------------------------------------------
# Targets y sufijos
.PHONY: all clean
#regla para hacer la libreria
all: $(EXEC_FAST) $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_HISTO) $(EXEC_PROBE) $(EXEC_TRACE)

$(EXEC_FAST): ivm_emu.c ivm_emu.h
	$(CC) $(CFLAGS) $< -o $@

$(EXEC_SEQ): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng

$(EXEC_HISTO): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DSTEPCOUNT -DNOOPT -DVERBOSE=1 -DHISTOGRAM

$(EXEC_PROBE): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DSTEPCOUNT -DNOOPT -DVERBOSE=2

$(EXEC_TRACE): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DSTEPCOUNT -DNOOPT -DVERBOSE=3

$(EXEC_PAR): ivm_emu.c ivm_io.c io_handler.c list.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DPARALLEL_OUTPUT -pthread

clean:
	-rm -fv $(EXEC_FAST) $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_HISTO) $(EXEC_PROBE) $(EXEC_TRACE)
