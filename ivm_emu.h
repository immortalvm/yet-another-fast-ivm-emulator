/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Authors:
  Eladio Gutierrez Carrasco
  Sergio Romero Montiel
  Oscar Plata Gonzalez

 Date: Mar 2020
*/

#ifndef __IVM_EMU_H
#define __IVM_EMU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#define IVM_BINARY_VERSION 2

// native insn
enum OPCODES {
	OPCODE_EXIT			= 0,
	OPCODE_NOP			= 1,
	OPCODE_JUMP			= 2,
	OPCODE_JZ_FWD       = 3,
	OPCODE_JZ_BACK      = 4,
	OPCODE_SET_SP       = 5,
	OPCODE_GET_PC       = 6,
	OPCODE_GET_SP       = 7,
	OPCODE_PUSH0        = 8,
	OPCODE_PUSH1        = 9,
	OPCODE_PUSH2        = 10,
	OPCODE_PUSH4        = 11,
	OPCODE_PUSH8        = 12,
	OPCODE_LOAD1        = 16,
	OPCODE_LOAD2        = 17,
	OPCODE_LOAD4        = 18,
	OPCODE_LOAD8        = 19,
	OPCODE_STORE1       = 20,
	OPCODE_STORE2       = 21,
	OPCODE_STORE4       = 22,
	OPCODE_STORE8       = 23,
	OPCODE_ADD          = 32,
	OPCODE_MUL          = 33,
	OPCODE_DIV          = 34,
	OPCODE_REM          = 35,
	OPCODE_LT           = 36,
	OPCODE_AND          = 40,
	OPCODE_OR           = 41,
	OPCODE_NOT          = 42,
	OPCODE_XOR          = 43,
	OPCODE_POW2         = 44,
    OPCODE_CHECK        = 48,
// recode also native insn to avoid overload on successive accesses
#if (defined(RECODE_NATIVE_INSN) || defined(RECODE_INSN))
	OPCODE_NEW_NOP,
	OPCODE_NEW_GET_PC,
	OPCODE_NEW_GET_SP,
	OPCODE_NEW_PUSH0,
	OPCODE_NEW_PUSH1,
	OPCODE_NEW_PUSH2,
	OPCODE_NEW_PUSH4,
	OPCODE_NEW_LT,
	OPCODE_NEW_XOR,
#endif
// NOP
#ifdef PATTERN_NOPN
	#if (NOP2_INSN > 0)
		OPCODE_NOP2,
	#endif
	#if (NOP4_INSN > 0)
		OPCODE_NOP4,
	#endif
	#if (NOP8_INSN > 0)
		OPCODE_NOP8,
	#endif
#endif
// GET_PC
#ifdef PATTERN_GETPC_PUSH8_ADD
	#if (LD1_PC_8_INSN > 0)
		OPCODE_LD1_PC_8,
	#endif
	#if (LD2_PC_8_INSN > 0)
		OPCODE_LD2_PC_8,
	#endif
	#if (LD4_PC_8_INSN > 0)
		OPCODE_LD4_PC_8,
	#endif
	#if (LD8_PC_8_INSN > 0)
		OPCODE_LD8_PC_8,
	#endif
	#if (ST1_PC_8_INSN > 0)
		OPCODE_ST1_PC_8,
	#endif
	#if (ST2_PC_8_INSN > 0)
		OPCODE_ST2_PC_8,
	#endif
	#if (ST4_PC_8_INSN > 0)
		OPCODE_ST4_PC_8,
	#endif
	#if (ST8_PC_8_INSN > 0)
		OPCODE_ST8_PC_8,
	#endif
	#if (PC_8_JUMP_INSN > 0)
		OPCODE_PC_8_JUMP,
	#endif
	#if (PC_8_INSN > 0)
		OPCODE_PC_8,
	#endif
#endif
#ifdef PATTERN_GETPC_PUSH4_ADD
	#if (LD1_PC_4_INSN > 0)
		OPCODE_LD1_PC_4,
	#endif
	#if (LD2_PC_4_INSN > 0)
		OPCODE_LD2_PC_4,
	#endif
	#if (LD4_PC_4_INSN > 0)
		OPCODE_LD4_PC_4,
	#endif
	#if (LD8_PC_4_INSN > 0)
		OPCODE_LD8_PC_4,
	#endif
	#if (ST1_PC_4_INSN > 0)
		OPCODE_ST1_PC_4,
	#endif
	#if (ST2_PC_4_INSN > 0)
		OPCODE_ST2_PC_4,
	#endif
	#if (ST4_PC_4_INSN > 0)
		OPCODE_ST4_PC_4,
	#endif
	#if (ST8_PC_4_INSN > 0)
		OPCODE_ST8_PC_4,
	#endif
	#if (PC_4_JUMP_INSN > 0)
		OPCODE_PC_4_JUMP,
	#endif
	#if (PC_4_INSN > 0)
		OPCODE_PC_4,
	#endif
#endif
#ifdef PATTERN_GETPC_PUSH2_ADD
	#if (LD1_PC_2_INSN > 0)
		OPCODE_LD1_PC_2,
	#endif
	#if (LD2_PC_2_INSN > 0)
		OPCODE_LD2_PC_2,
	#endif
	#if (LD4_PC_2_INSN > 0)
		OPCODE_LD4_PC_2,
	#endif
	#if (LD8_PC_2_INSN > 0)
		OPCODE_LD8_PC_2,
	#endif
	#if (ST1_PC_2_INSN > 0)
		OPCODE_ST1_PC_2,
	#endif
	#if (ST2_PC_2_INSN > 0)
		OPCODE_ST2_PC_2,
	#endif
	#if (ST4_PC_2_INSN > 0)
		OPCODE_ST4_PC_2,
	#endif
	#if (ST8_PC_2_INSN > 0)
		OPCODE_ST8_PC_2,
	#endif
	#if (PC_2_JUMP_INSN > 0)
		OPCODE_PC_2_JUMP,
	#endif
	#if (PC_2_INSN > 0)
		OPCODE_PC_2,
	#endif
#endif
#ifdef PATTERN_GETPC_PUSH1_ADD
	#if (LD1_PC_1_INSN > 0)
		OPCODE_LD1_PC_1,
	#endif
	#if (LD2_PC_1_INSN > 0)
		OPCODE_LD2_PC_1,
	#endif
	#if (LD4_PC_1_INSN > 0)
		OPCODE_LD4_PC_1,
	#endif
	#if (LD8_PC_1_INSN > 0)
		OPCODE_LD8_PC_1,
	#endif
	#if (ST1_PC_1_INSN > 0)
		OPCODE_ST1_PC_1,
	#endif
	#if (ST2_PC_1_INSN > 0)
		OPCODE_ST2_PC_1,
	#endif
	#if (ST4_PC_1_INSN > 0)
		OPCODE_ST4_PC_1,
	#endif
	#if (ST8_PC_1_INSN > 0)
		OPCODE_ST8_PC_1,
	#endif
	#if (PC_1_JUMP_INSN > 0)
		OPCODE_PC_1_JUMP,
	#endif
	#if (PC_1_NOP_INSN > 0)
		OPCODE_PC_1_NOP,
	#endif
	#if (PC_OFFSET_INSN > 0)
		OPCODE_PC_OFFSET,
	#endif
#endif
// GET_SP
#ifdef PATTERN_GETSP_PUSH1_ADD
	#if (LD1_SP_1_INSN > 0)
		OPCODE_LD1_SP_1,
	#endif
	#if (LD2_SP_1_INSN > 0)
		OPCODE_LD2_SP_1,
	#endif
	#if (LD4_SP_1_INSN > 0)
		OPCODE_LD4_SP_1,
	#endif
	#if (LD8_SP_1_INSN > 0)
		OPCODE_LD8_SP_1,
	#endif
	#if (ST1_SP_1_INSN > 0)
		OPCODE_ST1_SP_1,
	#endif
	#if (ST2_SP_1_INSN > 0)
		OPCODE_ST2_SP_1,
	#endif
	#if (ST4_SP_1_INSN > 0)
		OPCODE_ST4_SP_1,
	#endif
	#if (ST8_SP_1_INSN > 0)
		OPCODE_ST8_SP_1,
	#endif
	#if (CHANGE_SP_INSN > 0)
		OPCODE_CHANGE_SP,
	#endif
	#if (SP_OFFSET_INSN > 0)
		OPCODE_SP_OFFSET,
	#endif
#endif
#ifdef PATTERN_GETSP_PUSH2_ADD
	#if (LD1_SP_2_INSN > 0)
		OPCODE_LD1_SP_2,
	#endif
	#if (LD2_SP_2_INSN > 0)
		OPCODE_LD2_SP_2,
	#endif
	#if (LD4_SP_2_INSN > 0)
		OPCODE_LD4_SP_2,
	#endif
	#if (LD8_SP_2_INSN > 0)
		OPCODE_LD8_SP_2,
	#endif
	#if (ST1_SP_2_INSN > 0)
		OPCODE_ST1_SP_2,
	#endif
	#if (ST2_SP_2_INSN > 0)
		OPCODE_ST2_SP_2,
	#endif
	#if (ST4_SP_2_INSN > 0)
		OPCODE_ST4_SP_2,
	#endif
	#if (ST8_SP_2_INSN > 0)
		OPCODE_ST8_SP_2,
	#endif
	#if (SP_2_INSN > 0)
		OPCODE_SP_2,
	#endif
#endif
#ifdef PATTERN_GETSP_PUSH1
	#if (SP_1_INSN > 0)
		OPCODE_SP_1,
	#endif
	#if (DEC_SP_1_INSN > 0)
		OPCODE_DEC_SP_1,
	#endif
#endif
// PATTERN_GETSP_PUSH4_ADD, PATTERN_GETSP_PUSH8_ADD -> unused
#ifdef PATTERN_GETSP_PUSH4_ADD
	#if (LD1_SP_4_INSN > 0)
		OPCODE_LD1_SP_4,
	#endif
	#if (LD2_SP_4_INSN > 0)
		OPCODE_LD2_SP_4,
	#endif
	#if (LD4_SP_4_INSN > 0)
		OPCODE_LD4_SP_4,
	#endif
	#if (LD8_SP_4_INSN > 0)
		OPCODE_LD8_SP_4,
	#endif
	#if (ST1_SP_4_INSN > 0)
		OPCODE_ST1_SP_4,
	#endif
	#if (ST2_SP_4_INSN > 0)
		OPCODE_ST2_SP_4,
	#endif
	#if (ST4_SP_4_INSN > 0)
		OPCODE_ST4_SP_4,
	#endif
	#if (ST8_SP_4_INSN > 0)
		OPCODE_ST8_SP_4,
	#endif
#endif
#ifdef PATTERN_GETSP_STORE
    #if (FAST_POP_INSN > 0)
        OPCODE_FAST_POP,
    #endif
    #if (FAST_POP2_INSN > 0)
        OPCODE_FAST_POP2,
    #endif
#endif
#ifdef PATTERN_PUSH0
	#if (SHORT_JUMPF_INSN > 0)
		OPCODE_SHORT_JUMPF,
	#endif
	#if (SHORT_JUMPB_INSN > 0)
		OPCODE_SHORT_JUMPB,
	#endif
	#if (XOR_0_INSN > 0)
		OPCODE_XOR_0,
	#endif
	#if (NOT_0_MUL_INSN > 0)
		OPCODE_NOT_0_MUL,
	#endif
    #if (PUSH0X2_INSN > 0)
        OPCODE_PUSH0X2,
    #endif
    #if (PUSH0X3_INSN > 0)
        OPCODE_PUSH0X3,
    #endif
    #if (PUSH0X4_INSN > 0)
        OPCODE_PUSH0X4,
    #endif
#endif
#ifdef PATTERN_PUSH1_ALU
	#if (LT_1_JZF_INSN > 0)
		OPCODE_LT_1_JZF,
	#endif
	#if (LT_1_JZB_INSN > 0)
		OPCODE_LT_1_JZB,
	#endif
	#if (NOT_1_ADD_INSN > 0)
		OPCODE_NOT_1_ADD,
	#endif
	#if (LT_1_NOT_INSN > 0)
		OPCODE_LT_1_NOT,
	#endif
	#if (LT_1_JNZF_INSN > 0)
		OPCODE_LT_1_JNZF,
	#endif
	#if (LT_1_JNZB_INSN > 0)
		OPCODE_LT_1_JNZB,
	#endif
#endif
#ifdef PATTERN_PUSH1_POW2
	#if (POW2_1_ADD_INSN > 0)
		OPCODE_POW2_1_ADD,
	#endif
	#if (POW2_1_MUL_INSN > 0)
		OPCODE_POW2_1_MUL,
	#endif
	#if (POW2_1_LT_INSN > 0)
		OPCODE_POW2_1_LT,
	#endif
	#if (POW2_1_DIV_INSN > 0)
		OPCODE_POW2_1_DIV,
	#endif
	#if (POW2_1_INSN > 0)
		OPCODE_POW2_1,
	#endif
#endif
#ifdef PATTERN_PUSH1N
	#if (PUSH1X2_INSN > 0)
		OPCODE_PUSH1X2,
	#endif
	#if (PUSH1X4_INSN > 0)
		OPCODE_PUSH1X4,
	#endif
#endif
#ifdef PATTERN_PUSH1_HIGH4
	#if (C1TOSTACK1_INSN > 0)
		OPCODE_C1TOSTACK1,
	#endif
	#if (C1TOSTACK2_INSN > 0)
		OPCODE_C1TOSTACK2,
	#endif
	#if (C1TOSTACK4_INSN > 0)
		OPCODE_C1TOSTACK4,
	#endif
	#if (C1TOSTACK8_INSN > 0)
		OPCODE_C1TOSTACK8,
	#endif
	#if (JUMP_PC_1_INSN > 0)
		OPCODE_JUMP_PC_1,
	#endif
#endif
#ifdef PATTERN_PUSH2
	#if (JUMP_PC_2_INSN > 0)
		OPCODE_JUMP_PC_2,
	#endif
	#if (C2TOSTACK1_INSN > 0)
		OPCODE_C2TOSTACK1,
	#endif
	#if (C2TOSTACK2_INSN > 0)
		OPCODE_C2TOSTACK2,
	#endif
	#if (C2TOSTACK4_INSN > 0)
		OPCODE_C2TOSTACK4,
	#endif
	#if (C2TOSTACK8_INSN > 0)
		OPCODE_C2TOSTACK8,
	#endif
#endif
#ifdef PATTERN_PUSH4
	#if (JUMP_PC_4_INSN > 0)
		OPCODE_JUMP_PC_4,
	#endif
#endif
#ifdef PATTERN_LT
	#if (LT_JZF_INSN > 0)
		OPCODE_LT_JZF,
	#endif
	#if (LT_NOT_JZF_INSN > 0)
		OPCODE_LT_NOT_JZF,
	#endif
	#if (LT_JZB_INSN > 0)
		OPCODE_LT_JZB,
	#endif
	#if (LT_NOT_JZB_INSN > 0)
		OPCODE_LT_NOT_JZB,
	#endif

#endif
#ifdef PATTERN_XOR
	#if (XOR_1_LT_INSN > 0)
		OPCODE_XOR_1_LT,
	#endif
#endif

// Break, trace and probe options
	OPCODE_BREAK      = 0xf0,
	OPCODE_TRACE      = 0xf1,
	OPCODE_PROBE      = 0xf2,
	OPCODE_PROBE_READ = 0xf3,


// native IO insn
    OPCODE_READ_CHAR   = 0xf8,
	OPCODE_PUT_BYTE   = 0xf9,
	OPCODE_PUT_CHAR   = 0xfa,
	OPCODE_ADD_SAMPLE = 0xfb,
	OPCODE_SET_PIXEL  = 0xfc,
	OPCODE_NEW_FRAME  = 0xfd,
	OPCODE_READ_PIXEL = 0xfe,
	OPCODE_READ_FRAME = 0xff,
};


// An instruction pattern exists if it defined as 1 or 2 (0 -> do not exist)
// Useful for histograms, so we set the pattern in the table even if the
//    instruction is not recoded (defined as 1)
#define ATTR_TABLE2(T,X,Y)	T[OPCODE_##X] = (insn_attr_t){ #X, Y }
#if ((VERBOSE>1) || defined(HISTOGRAM))
	#define ATTR_TABLE1(T,X,Y)	T[OPCODE_##X] = (insn_attr_t){ #X, Y }
#else
	#define ATTR_TABLE1(T,X,Y)
#endif
#define ATTR_TABLE0(T,X,Y)
#define ATTRIBUTE(T,X,Y)	concat(ATTR_TABLE,X##_INSN)(T,X,Y)
#define ATTR_NATIVE(T,X,Y)	ATTR_TABLE2(T,X,Y)


#define init_attributes_native_insn(A) \
ATTR_NATIVE(A,EXIT,0); \
ATTR_NATIVE(A,NOP,0); \
ATTR_NATIVE(A,JUMP,0); \
ATTR_NATIVE(A,JZ_FWD,1); \
ATTR_NATIVE(A,JZ_BACK, 1); \
ATTR_NATIVE(A,SET_SP,0); \
ATTR_NATIVE(A,GET_PC,0); \
ATTR_NATIVE(A,GET_SP,0); \
ATTR_NATIVE(A,PUSH0,0); \
ATTR_NATIVE(A,PUSH1,1); \
ATTR_NATIVE(A,PUSH2,2); \
ATTR_NATIVE(A,PUSH4,4); \
ATTR_NATIVE(A,PUSH8,8); \
ATTR_NATIVE(A,LOAD1,0); \
ATTR_NATIVE(A,LOAD2,0); \
ATTR_NATIVE(A,LOAD4,0); \
ATTR_NATIVE(A,LOAD8,0); \
ATTR_NATIVE(A,STORE1,0); \
ATTR_NATIVE(A,STORE2,0); \
ATTR_NATIVE(A,STORE4,0); \
ATTR_NATIVE(A,STORE8,0); \
ATTR_NATIVE(A,ADD,0); \
ATTR_NATIVE(A,MUL,0); \
ATTR_NATIVE(A,DIV,0); \
ATTR_NATIVE(A,REM,0); \
ATTR_NATIVE(A,LT,0); \
ATTR_NATIVE(A,AND,0); \
ATTR_NATIVE(A,OR,0); \
ATTR_NATIVE(A,NOT,0); \
ATTR_NATIVE(A,XOR,0); \
ATTR_NATIVE(A,POW2,0); \
ATTR_NATIVE(A,CHECK,0); \
ATTR_NATIVE(A,READ_CHAR,0); \
ATTR_NATIVE(A,PUT_BYTE,0); \
ATTR_NATIVE(A,PUT_CHAR,0); \
ATTR_NATIVE(A,ADD_SAMPLE,0); \
ATTR_NATIVE(A,NEW_FRAME,0); \
ATTR_NATIVE(A,SET_PIXEL,0); \
ATTR_NATIVE(A,READ_PIXEL,0); \
ATTR_NATIVE(A,READ_FRAME,0);


#if (defined(RECODE_NATIVE_INSN) || defined(RECODE_INSN))
#define init_attributes_new_native_insn(A)	\
ATTRIBUTE(A,NEW_NOP,0); \
ATTRIBUTE(A,NEW_GET_PC,0); \
ATTRIBUTE(A,NEW_GET_SP,0); \
ATTRIBUTE(A,NEW_PUSH0,0); \
ATTRIBUTE(A,NEW_PUSH1,1); \
ATTRIBUTE(A,NEW_PUSH2,2); \
ATTRIBUTE(A,NEW_PUSH4,4); \
ATTRIBUTE(A,NEW_LT,4); \
ATTRIBUTE(A,NEW_XOR,4);
#else
#define init_attributes_new_native_insn(A)
#endif

#ifdef PATTERN_NOPN
#define init_attributes_pattern_nopn(A)	\
ATTRIBUTE(A,NOP2,1); \
ATTRIBUTE(A,NOP4,3); \
ATTRIBUTE(A,NOP8,7);
#else
#define init_attributes_pattern_nopn(A)
#endif

#ifdef PATTERN_GETPC_PUSH1_ADD
#define init_attributes_pattern_getpc_push1_add(A)	\
ATTRIBUTE(A,LD1_PC_1,4); \
ATTRIBUTE(A,LD2_PC_1,4); \
ATTRIBUTE(A,LD4_PC_1,4); \
ATTRIBUTE(A,LD8_PC_1,4); \
ATTRIBUTE(A,ST1_PC_1,4); \
ATTRIBUTE(A,ST2_PC_1,4); \
ATTRIBUTE(A,ST4_PC_1,4); \
ATTRIBUTE(A,ST8_PC_1,4); \
ATTRIBUTE(A,PC_1_JUMP,4); \
ATTRIBUTE(A,PC_1_NOP,4); \
ATTRIBUTE(A,PC_OFFSET,3);
#else
#define init_attributes_pattern_getpc_push1_add(A)
#endif

#ifdef PATTERN_GETPC_PUSH2_ADD
#define init_attributes_pattern_getpc_push2_add(A)	\
ATTRIBUTE(A,LD1_PC_2,5); \
ATTRIBUTE(A,LD2_PC_2,5); \
ATTRIBUTE(A,LD4_PC_2,5); \
ATTRIBUTE(A,LD8_PC_2,5); \
ATTRIBUTE(A,ST1_PC_2,5); \
ATTRIBUTE(A,ST2_PC_2,5); \
ATTRIBUTE(A,ST4_PC_2,5); \
ATTRIBUTE(A,ST8_PC_2,5); \
ATTRIBUTE(A,PC_2_JUMP,5); \
ATTRIBUTE(A,PC_2,3);
#else
#define init_attributes_pattern_getpc_push2_add(A)
#endif

#ifdef PATTERN_GETPC_PUSH4_ADD
#define init_attributes_pattern_getpc_push4_add(A)	\
ATTRIBUTE(A,LD1_PC_4,7); \
ATTRIBUTE(A,LD2_PC_4,7); \
ATTRIBUTE(A,LD4_PC_4,7); \
ATTRIBUTE(A,LD8_PC_4,7); \
ATTRIBUTE(A,ST1_PC_4,7); \
ATTRIBUTE(A,ST2_PC_4,7); \
ATTRIBUTE(A,ST4_PC_4,7); \
ATTRIBUTE(A,ST8_PC_4,7); \
ATTRIBUTE(A,PC_4_JUMP,7); \
ATTRIBUTE(A,PC_4,5);
#else
#define init_attributes_pattern_getpc_push4_add(A)
#endif

#ifdef PATTERN_GETPC_PUSH8_ADD
#define init_attributes_pattern_getpc_push8_add(A)	\
ATTRIBUTE(A,LD1_PC_8,11); \
ATTRIBUTE(A,LD2_PC_8,11); \
ATTRIBUTE(A,LD4_PC_8,11); \
ATTRIBUTE(A,LD8_PC_8,11); \
ATTRIBUTE(A,ST1_PC_8,11); \
ATTRIBUTE(A,ST2_PC_8,11); \
ATTRIBUTE(A,ST4_PC_8,11); \
ATTRIBUTE(A,ST8_PC_8,11); \
ATTRIBUTE(A,PC_8_JUMP,11); \
ATTRIBUTE(A,PC_8,9);
#else
#define init_attributes_pattern_getpc_push8_add(A)
#endif

#ifdef PATTERN_GETSP_PUSH1_ADD
#define init_attributes_pattern_getsp_push1_add(A)	\
ATTRIBUTE(A,LD1_SP_1,4); \
ATTRIBUTE(A,LD2_SP_1,4); \
ATTRIBUTE(A,LD4_SP_1,4); \
ATTRIBUTE(A,LD8_SP_1,4); \
ATTRIBUTE(A,ST1_SP_1,4); \
ATTRIBUTE(A,ST2_SP_1,4); \
ATTRIBUTE(A,ST4_SP_1,4); \
ATTRIBUTE(A,ST8_SP_1,4); \
ATTRIBUTE(A,CHANGE_SP,4); \
ATTRIBUTE(A,SP_OFFSET,3);
#else
#define init_attributes_pattern_getsp_push1_add(A)
#endif

#ifdef PATTERN_GETSP_PUSH2_ADD
//getsp_push not followed by add
#define init_attributes_pattern_getsp_push2_add(A)	\
ATTRIBUTE(A,LD1_SP_2,5); \
ATTRIBUTE(A,LD2_SP_2,5); \
ATTRIBUTE(A,LD4_SP_2,5); \
ATTRIBUTE(A,LD8_SP_2,5); \
ATTRIBUTE(A,ST1_SP_2,5); \
ATTRIBUTE(A,ST2_SP_2,5); \
ATTRIBUTE(A,ST4_SP_2,5); \
ATTRIBUTE(A,ST8_SP_2,5); \
ATTRIBUTE(A,SP_2,3);
#else
#define init_attributes_pattern_getsp_push2_add(A)
#endif

// PATTERN_GETSP_PUSH4_ADD, PATTERN_GETSP_PUSH8_ADD unused
#ifdef PATTERN_GETSP_PUSH4_ADD
ATTRIBUTE(A,LD1_SP_4,7); \
ATTRIBUTE(A,LD2_SP_4,7); \
ATTRIBUTE(A,LD4_SP_4,7); \
ATTRIBUTE(A,LD8_SP_4,7); \
ATTRIBUTE(A,ST1_SP_4,7); \
ATTRIBUTE(A,ST2_SP_4,7); \
ATTRIBUTE(A,ST4_SP_4,7); \
ATTRIBUTE(A,ST8_SP_4,7);
#endif

#ifdef PATTERN_GETSP_PUSH1
#define init_attributes_pattern_getsp_push1(A)	\
ATTRIBUTE(A,DEC_SP_1,2); \
ATTRIBUTE(A,SP_1,2);
#else
#define init_attributes_pattern_getsp_push1(A)
#endif

#ifdef PATTERN_GETSP_STORE
#define init_attributes_pattern_getsp_store(A)  \
ATTRIBUTE(A,FAST_POP,1); \
ATTRIBUTE(A,FAST_POP2,3);
#else
#define init_attributes_pattern_getsp_store(A)
#endif

#ifdef PATTERN_PUSH0
//push0/xor
#define init_attributes_pattern_push0(A)	\
ATTRIBUTE(A,SHORT_JUMPF,2); \
ATTRIBUTE(A,SHORT_JUMPB,2); \
ATTRIBUTE(A,XOR_0,1);       \
ATTRIBUTE(A,NOT_0_MUL,2);   \
ATTRIBUTE(A,PUSH0X2,1);     \
ATTRIBUTE(A,PUSH0X3,2);     \
ATTRIBUTE(A,PUSH0X4,3);
#else
#define init_attributes_pattern_push0(A)
#endif

#ifdef PATTERN_PUSH1_ALU
#define init_attributes_pattern_push1_alu(A)	\
ATTRIBUTE(A,LT_1_JZF,4);	\
ATTRIBUTE(A,LT_1_JZB,4);	\
ATTRIBUTE(A,LT_1_NOT,4);    \
ATTRIBUTE(A,LT_1_JNZF,5);   \
ATTRIBUTE(A,LT_1_JNZB,5);   \
ATTRIBUTE(A,NOT_1_ADD,4);
#else
#define init_attributes_pattern_push1_alu(A)
#endif

#ifdef PATTERN_PUSH1_POW2
#define init_attributes_pattern_push1_pow2(A)	\
ATTRIBUTE(A,POW2_1_ADD,3);  \
ATTRIBUTE(A,POW2_1_MUL,3);  \
ATTRIBUTE(A,POW2_1_LT,3);   \
ATTRIBUTE(A,POW2_1_DIV,3);  \
ATTRIBUTE(A,POW2_1,2);
#else
#define init_attributes_pattern_push1_pow2(A)
#endif

#ifdef PATTERN_PUSH1N
#define init_attributes_pattern_push1n(A)	\
ATTRIBUTE(A,PUSH1X2,3); \
ATTRIBUTE(A,PUSH1X4,7);
#else
#define init_attributes_pattern_push1n(A)
#endif

#ifdef PATTERN_PUSH1_HIGH4
#define init_attributes_pattern_high4(A)	\
ATTRIBUTE(A,C1TOSTACK1,6); \
ATTRIBUTE(A,C1TOSTACK2,6); \
ATTRIBUTE(A,C1TOSTACK4,6); \
ATTRIBUTE(A,C1TOSTACK8,6); \
ATTRIBUTE(A,JUMP_PC_1,4);
#else
#define init_attributes_pattern_high4(A)
#endif

#ifdef PATTERN_PUSH2
#define init_attributes_pattern_push2(A)	\
ATTRIBUTE(A,C2TOSTACK1,6); \
ATTRIBUTE(A,C2TOSTACK2,6); \
ATTRIBUTE(A,C2TOSTACK4,6); \
ATTRIBUTE(A,C2TOSTACK8,6); \
ATTRIBUTE(A,JUMP_PC_2,5);
#else
#define init_attributes_pattern_push2(A)
#endif

#ifdef PATTERN_PUSH4
#define init_attributes_pattern_push4(A)	\
ATTRIBUTE(A,JUMP_PC_4,7);
#else
#define init_attributes_pattern_push4(A)
#endif

#ifdef PATTERN_LT
#define init_attributes_pattern_lt(A)	\
ATTRIBUTE(A,LT_JZF,2);		\
ATTRIBUTE(A,LT_NOT_JZF,3);  \
ATTRIBUTE(A,LT_JZB,2);		\
ATTRIBUTE(A,LT_NOT_JZB,3);
#else
#define init_attributes_pattern_lt(A)
#endif

#ifdef PATTERN_XOR
#define init_attributes_pattern_xor(A)	\
ATTRIBUTE(A,XOR_1_LT,3);
#else
#define init_attributes_pattern_xor(A)
#endif

#define init_attributes_trace_insn(A)	\
ATTR_NATIVE(A,BREAK,0); \
ATTR_NATIVE(A,TRACE,1); \
ATTR_NATIVE(A,PROBE,1); \
ATTR_NATIVE(A,PROBE_READ,0);


#define init_insn_attributes(A)	\
	do {	\
		init_attributes_native_insn(A);				\
		init_attributes_new_native_insn(A);			\
		init_attributes_pattern_nopn(A);			\
		init_attributes_pattern_getpc_push1_add(A);	\
		init_attributes_pattern_getpc_push2_add(A);	\
		init_attributes_pattern_getpc_push4_add(A);	\
		init_attributes_pattern_getpc_push8_add(A);	\
		init_attributes_pattern_getsp_push1_add(A);	\
		init_attributes_pattern_getsp_push2_add(A);	\
		init_attributes_pattern_getsp_push1(A);		\
		init_attributes_pattern_getsp_store(A);     \
		init_attributes_pattern_push0(A);			\
		init_attributes_pattern_push1_alu(A);		\
		init_attributes_pattern_push1_pow2(A);		\
		init_attributes_pattern_push1n(A);			\
		init_attributes_pattern_high4(A);			\
		init_attributes_pattern_push2(A);			\
		init_attributes_pattern_push4(A);			\
		init_attributes_pattern_lt(A);				\
		init_attributes_pattern_xor(A);				\
		init_attributes_trace_insn(A);				\
	} while(0)



// An instruction is recoded if it defined 2 (1 use pattern but not recode)
#define LABEL_TABLE2(T,X)	T[OPCODE_##X] = &&X
#define LABEL_TABLE1(T,X)
#define LABEL_TABLE0(T,X)
#ifdef RECODE_INSN
	#define BIND_LABEL(T,X)		concat(LABEL_TABLE,X##_INSN)(T,X)
#else
	#define BIND_LABEL(T,X)
#endif
#define BIND_NATIVE(T,X)	LABEL_TABLE2(T,X)

#define init_addr_native_insn(B)	\
BIND_NATIVE(B,EXIT); \
BIND_NATIVE(B,NOP); \
BIND_NATIVE(B,JUMP); \
BIND_NATIVE(B,JZ_FWD); \
BIND_NATIVE(B,JZ_BACK); \
BIND_NATIVE(B,SET_SP); \
BIND_NATIVE(B,GET_PC); \
BIND_NATIVE(B,GET_SP); \
BIND_NATIVE(B,PUSH0); \
BIND_NATIVE(B,PUSH1); \
BIND_NATIVE(B,PUSH2); \
BIND_NATIVE(B,PUSH4); \
BIND_NATIVE(B,PUSH8); \
BIND_NATIVE(B,LOAD1); \
BIND_NATIVE(B,LOAD2); \
BIND_NATIVE(B,LOAD4); \
BIND_NATIVE(B,LOAD8); \
BIND_NATIVE(B,STORE1); \
BIND_NATIVE(B,STORE2); \
BIND_NATIVE(B,STORE4); \
BIND_NATIVE(B,STORE8); \
BIND_NATIVE(B,ADD); \
BIND_NATIVE(B,MUL); \
BIND_NATIVE(B,DIV); \
BIND_NATIVE(B,REM); \
BIND_NATIVE(B,LT); \
BIND_NATIVE(B,AND); \
BIND_NATIVE(B,OR); \
BIND_NATIVE(B,NOT); \
BIND_NATIVE(B,XOR); \
BIND_NATIVE(B,POW2); \
BIND_NATIVE(B,CHECK); \
BIND_NATIVE(B,READ_CHAR); \
BIND_NATIVE(B,PUT_BYTE); \
BIND_NATIVE(B,PUT_CHAR); \
BIND_NATIVE(B,ADD_SAMPLE); \
BIND_NATIVE(B,NEW_FRAME); \
BIND_NATIVE(B,SET_PIXEL); \
BIND_NATIVE(B,READ_PIXEL); \
BIND_NATIVE(B,READ_FRAME);

#ifdef RECODE_NATIVE_INSN
#define init_addr_new_native_insn(B) \
BIND_LABEL(B,NEW_NOP); \
BIND_LABEL(B,NEW_GET_PC); \
BIND_LABEL(B,NEW_GET_SP); \
BIND_LABEL(B,NEW_PUSH0); \
BIND_LABEL(B,NEW_PUSH1); \
BIND_LABEL(B,NEW_PUSH2); \
BIND_LABEL(B,NEW_PUSH4); \
BIND_LABEL(B,NEW_LT); \
BIND_LABEL(B,NEW_XOR);
#else
#define init_addr_new_native_insn(B)
#endif

#ifdef PATTERN_NOPN
#define init_addr_pattern_nopn(B)				\
BIND_LABEL(B,NOP2); \
BIND_LABEL(B,NOP4); \
BIND_LABEL(B,NOP8);
#else
#define init_addr_pattern_nopn(B)
#endif

#ifdef PATTERN_GETPC_PUSH1_ADD
#define init_addr_pattern_getpc_push1_add(B)	\
BIND_LABEL(B,LD1_PC_1); \
BIND_LABEL(B,LD2_PC_1); \
BIND_LABEL(B,LD4_PC_1); \
BIND_LABEL(B,LD8_PC_1); \
BIND_LABEL(B,ST1_PC_1); \
BIND_LABEL(B,ST2_PC_1); \
BIND_LABEL(B,ST4_PC_1); \
BIND_LABEL(B,ST8_PC_1); \
BIND_LABEL(B,PC_OFFSET); \
BIND_LABEL(B,PC_1_NOP); \
BIND_LABEL(B,PC_1_JUMP);
#else
#define init_addr_pattern_getpc_push1_add(B)
#endif

#ifdef PATTERN_GETPC_PUSH2_ADD
#define init_addr_pattern_getpc_push2_add(B)	\
BIND_LABEL(B,LD1_PC_2); \
BIND_LABEL(B,LD2_PC_2); \
BIND_LABEL(B,LD4_PC_2); \
BIND_LABEL(B,LD8_PC_2); \
BIND_LABEL(B,ST1_PC_2); \
BIND_LABEL(B,ST2_PC_2); \
BIND_LABEL(B,ST4_PC_2); \
BIND_LABEL(B,ST8_PC_2); \
BIND_LABEL(B,PC_2_JUMP); \
BIND_LABEL(B,PC_2);
#else
#define init_addr_pattern_getpc_push2_add(B)
#endif

#ifdef PATTERN_GETPC_PUSH4_ADD
#define init_addr_pattern_getpc_push4_add(B)	\
BIND_LABEL(B,LD1_PC_4); \
BIND_LABEL(B,LD2_PC_4); \
BIND_LABEL(B,LD4_PC_4); \
BIND_LABEL(B,LD8_PC_4); \
BIND_LABEL(B,ST1_PC_4); \
BIND_LABEL(B,ST2_PC_4); \
BIND_LABEL(B,ST4_PC_4); \
BIND_LABEL(B,ST8_PC_4); \
BIND_LABEL(B,PC_4_JUMP); \
BIND_LABEL(B,PC_4);
#else
#define init_addr_pattern_getpc_push4_add(B)
#endif

#ifdef PATTERN_GETPC_PUSH8_ADD
#define init_addr_pattern_getpc_push8_add(B)	\
BIND_LABEL(B,LD1_PC_8); \
BIND_LABEL(B,LD2_PC_8); \
BIND_LABEL(B,LD4_PC_8); \
BIND_LABEL(B,LD8_PC_8); \
BIND_LABEL(B,ST1_PC_8); \
BIND_LABEL(B,ST2_PC_8); \
BIND_LABEL(B,ST4_PC_8); \
BIND_LABEL(B,ST8_PC_8); \
BIND_LABEL(B,PC_8_JUMP); \
BIND_LABEL(B,PC_8);
#else
#define init_addr_pattern_getpc_push8_add(B)
#endif

#ifdef PATTERN_GETSP_PUSH1_ADD
#define init_addr_pattern_getsp_push1_add(B)	\
BIND_LABEL(B,LD1_SP_1); \
BIND_LABEL(B,LD2_SP_1); \
BIND_LABEL(B,LD4_SP_1); \
BIND_LABEL(B,LD8_SP_1); \
BIND_LABEL(B,ST1_SP_1); \
BIND_LABEL(B,ST2_SP_1); \
BIND_LABEL(B,ST4_SP_1); \
BIND_LABEL(B,ST8_SP_1); \
BIND_LABEL(B,CHANGE_SP); \
BIND_LABEL(B,SP_OFFSET);
#else
#define init_addr_pattern_getsp_push1_add(B)
#endif

#ifdef PATTERN_GETSP_PUSH2_ADD
#define init_addr_pattern_getsp_push2_add(B)	\
BIND_LABEL(B,LD1_SP_2); \
BIND_LABEL(B,LD2_SP_2); \
BIND_LABEL(B,LD4_SP_2); \
BIND_LABEL(B,LD8_SP_2); \
BIND_LABEL(B,ST1_SP_2); \
BIND_LABEL(B,ST2_SP_2); \
BIND_LABEL(B,ST4_SP_2); \
BIND_LABEL(B,ST8_SP_2); \
BIND_LABEL(B,SP_2);
#else
#define init_addr_pattern_getsp_push2_add(B)
#endif

#ifdef PATTERN_GETSP_PUSH1
#define init_addr_pattern_getsp_push1(B)		\
BIND_LABEL(B,DEC_SP_1); \
BIND_LABEL(B,SP_1);
#else
#define init_addr_pattern_getsp_push1(B)
#endif

#ifdef PATTERN_GETSP_STORE
#define init_addr_pattern_getsp_store(B)    \
BIND_LABEL(B,FAST_POP); \
BIND_LABEL(B,FAST_POP2);
#else
#define init_addr_pattern_getsp_store(B)
#endif

#ifdef PATTERN_PUSH0
#define init_addr_pattern_push0(B)			\
BIND_LABEL(B,SHORT_JUMPF); \
BIND_LABEL(B,SHORT_JUMPB); \
BIND_LABEL(B,XOR_0); \
BIND_LABEL(B,NOT_0_MUL); \
BIND_LABEL(B,PUSH0X2); \
BIND_LABEL(B,PUSH0X3); \
BIND_LABEL(B,PUSH0X4);
#else
#define init_addr_pattern_push0(B)
#endif

#ifdef PATTERN_PUSH1_ALU
#define init_addr_pattern_push1_alu(B)		\
BIND_LABEL(B,LT_1_JZF);     \
BIND_LABEL(B,LT_1_JZB);     \
BIND_LABEL(B,LT_1_NOT);	    \
BIND_LABEL(B,LT_1_JNZF);	\
BIND_LABEL(B,LT_1_JNZB);	\
BIND_LABEL(B,NOT_1_ADD);
#else
#define init_addr_pattern_push1_alu(B)
#endif

#ifdef PATTERN_PUSH1_POW2
#define init_addr_pattern_push1_pow2(B)		\
BIND_LABEL(B,POW2_1_ADD); \
BIND_LABEL(B,POW2_1_MUL); \
BIND_LABEL(B,POW2_1_LT); \
BIND_LABEL(B,POW2_1_DIV); \
BIND_LABEL(B,POW2_1);
#else
#define init_addr_pattern_push1_pow2(B)
#endif

#ifdef PATTERN_PUSH1N
#define init_addr_pattern_push1n(B)			\
BIND_LABEL(B,PUSH1X2);					\
BIND_LABEL(B,PUSH1X4);
#else
#define init_addr_pattern_push1n(B)
#endif

#ifdef PATTERN_PUSH1_HIGH4
#define init_addr_pattern_high4(B)			\
BIND_LABEL(B,JUMP_PC_1); \
BIND_LABEL(B,C1TOSTACK1); \
BIND_LABEL(B,C1TOSTACK2); \
BIND_LABEL(B,C1TOSTACK4); \
BIND_LABEL(B,C1TOSTACK8);
#else
#define init_addr_pattern_high4(B)
#endif

#ifdef PATTERN_PUSH2
#define init_addr_pattern_push2(B)			\
BIND_LABEL(B,JUMP_PC_2); \
BIND_LABEL(B,C2TOSTACK1); \
BIND_LABEL(B,C2TOSTACK2); \
BIND_LABEL(B,C2TOSTACK4); \
BIND_LABEL(B,C2TOSTACK8);
#else
#define init_addr_pattern_push2(B)
#endif

#ifdef PATTERN_PUSH4
#define init_addr_pattern_push4(B)			\
BIND_LABEL(B,JUMP_PC_4);
#else
#define init_addr_pattern_push4(B)
#endif

#ifdef PATTERN_LT
#define init_addr_pattern_lt(B)			\
BIND_LABEL(B,LT_JZF);	    \
BIND_LABEL(B,LT_NOT_JZF);   \
BIND_LABEL(B,LT_JZB);	    \
BIND_LABEL(B,LT_NOT_JZB);
#else
#define init_addr_pattern_lt(B)
#endif

#ifdef PATTERN_XOR
#define init_addr_pattern_xor(B)			\
BIND_LABEL(B,XOR_1_LT);
#else
#define init_addr_pattern_xor(B)
#endif

#define init_addr_trace_insn(B) \
BIND_NATIVE(B,BREAK);   \
BIND_NATIVE(B,TRACE);   \
BIND_NATIVE(B,PROBE);   \
BIND_NATIVE(B,PROBE_READ);

#define init_insn_addr(B)	\
	do {	\
		init_addr_native_insn(B);				\
		init_addr_new_native_insn(B);			\
		init_addr_pattern_nopn(B);				\
		init_addr_pattern_getpc_push1_add(B);	\
		init_addr_pattern_getpc_push2_add(B);	\
		init_addr_pattern_getpc_push4_add(B);	\
		init_addr_pattern_getpc_push8_add(B);	\
		init_addr_pattern_getsp_push1_add(B);	\
		init_addr_pattern_getsp_push2_add(B);	\
		init_addr_pattern_getsp_push1(B);		\
		init_addr_pattern_getsp_store(B);       \
		init_addr_pattern_push0(B);				\
		init_addr_pattern_push1_alu(B);			\
		init_addr_pattern_push1_pow2(B);		\
		init_addr_pattern_push1n(B);			\
		init_addr_pattern_high4(B);				\
		init_addr_pattern_push2(B);				\
		init_addr_pattern_push4(B);				\
		init_addr_pattern_lt(B);				\
		init_addr_pattern_xor(B);				\
		init_addr_trace_insn(B);				\
	} while(0)


// Useful defines
#define xcat(a,b)	a##b
#define concat(...)	xcat(__VA_ARGS__)

#define MIN(a,b)	(((a)<(b))?(a):(b))
#define MAX(a,b)	(((a)>(b))?(a):(b))

// Bit masks
#define BITMASK(N)	((1UL<<N) - 1)
#define BITMASK8	BITMASK(8)
#define BITMASK16	BITMASK(16)
#define BITMASK32	BITMASK(32)

// Type for words (stack unit)
#define WORD_T uint64_t

// Bytes per word
#define BYTESPERWORD (sizeof(WORD_T))

//void push(WORD_T v);
//WORD_T pop();
//char* idx2addr(unsigned long i);
//unsigned long addr2idx(char* p);

#endif
