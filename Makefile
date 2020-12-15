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
EXEC_FAST	:=ivm_emu_no_io
EXEC_SEQ	:=ivm_emu
EXEC_PAR	:=ivm_emu_parallel
EXEC_HISTO  :=ivm_histo
EXEC_TRACE2 :=ivm_trace2
EXEC_TRACE3 :=ivm_trace3
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
all: $(EXEC_FAST) $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_HISTO) $(EXEC_TRACE2) $(EXEC_TRACE3)

$(EXEC_FAST): ivm_emu.c ivm_emu.h
	$(CC) $(CFLAGS) $< -o $@

$(EXEC_SEQ): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng

$(EXEC_HISTO): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DNOOPT -DVERBOSE=1 -DHISTOGRAM

$(EXEC_TRACE2): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DNOOPT -DVERBOSE=2

$(EXEC_TRACE3): ivm_emu.c ivm_io.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DNOOPT -DVERBOSE=3

$(EXEC_PAR): ivm_emu.c ivm_io.c io_handler.c list.c
	$(CC) $(CFLAGS) $^ -o $@ -DWITH_IO -lpng -DPARALLEL_OUTPUT -pthread

clean:
	-rm -fv $(EXEC_FAST) $(EXEC_SEQ) $(EXEC_PAR) $(EXEC_HISTO) $(EXEC_TRACE2) $(EXEC_TRACE3)
