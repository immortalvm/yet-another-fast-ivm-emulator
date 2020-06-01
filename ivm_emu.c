/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Original emulator:
 Authors:
  Eladio Gutierrez Carrasco
  Sergio Romero Montiel
  Oscar Plata Gonzalez

 Date: Mar 2020


 Input/output version:
 IO stuff in file ivm_io.c, fucntion extracted from Ivar Rummelhoff code in:
 github.com/preservationvm/ivm-implementations/blob/master/OtherMachines/vm.c
 Requeriments:
 * install libpng: sudo apt-get install libpng-dev
 * compile ivm_emu with options: -DWITH_IO -lpng and link with ivm_io

 Date: May 2020


 Parallel version:
 Output frames (and other files) are enqueued and written by additional thread/s
 Requeriments:
 * pthreads library
 * compile with options: -DWITH_IO -lpng -DPARALLEL_OUTPUT -pthread [-DNUM_THREADS]
 * and link with: ivm_io (compiled with -DPARALLEL_OUTPUT), io_handler, list
 
 Date: May 2020


 Some ideas from http://www.w3group.de/stable.html

*/

/*
 Compile with:
    make # generates the next three executables
    gcc -Ofast ivm_emu.c   # The fastest one without input/ouput
	gcc -Ofast -DWITH_IO   ivm_emu.c ivm_io.c -lpng  # Enable IO instruction
    gcc -Ofast -DWITH_IO   ivm_emu.c ivm_io.c io_handler.c list.c -lpng -DPARALLEL_OUTPUT -pthread
    
    gcc -Ofast -DVERBOSE=1 ivm_emu.c  # Enable verbose
    gcc -Ofast -DVERBOSE=2 ivm_emu.c  #
    gcc -Ofast -DVERBOSE=3 ivm_emu.c  #
    gcc -Ofast -DSTEPCOUNT ivm_emu.c  # Enable instruction count
    gcc -Ofast -DNOOPT     ivm_emu.c  # Disable optimizations
    gcc -Ofast -DHISTOGRAM ivm_emu.c  # Enable insn. pattern histogram

 Number of threads for the parallel version:
 * Default: 8
 * if compiled with -DNUM_THREADS=N1, N1 is used instead of the default value
 * if environment variable export NUM_THREADS=N2, N2 is used instead of N1 or default
 * In any case, the parallel version uses at least 2 threads, in general:
            1 thread for emulation and (N-1) thread for io
*/

// Version
#ifdef WITH_IO
    #ifdef PARALLEL_OUTPUT
    #define VERSION  "v1.0-fast-io-parallel"
    #else
    #define VERSION  "v1.0-fast-io"
    #endif
#else
    #define VERSION  "v1.0-fast"
#endif

////////////////////////////////////////////////////////////////////////////////
// Disable optimizations: -DNOOPT
#if defined(NOOPT)
#define OPTENABLED  0
#else
#define OPTENABLED  1
#endif


////////////////////////////////////////////////////////////////////////////////
// The FOLLOWING is only available if OPTENABLED==1
// defines before include emulator header file
//
//        //////////////////////////////////
//        //                              //
//        //   THIS IS THE CONTROL PANEL  //
//        //                              //
//        //////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

#if OPTENABLED
    // define RECODE_INSN to make instruction recodification
    // undef to only use insn patterns
    #define RECODE_INSN
    #define RECODE_NATIVE_INSN
    //-- define PATTERNS to be used
    #define PATTERN_NOPN
    #define PATTERN_GETPC_PUSH8_ADD
    #define PATTERN_GETPC_PUSH4_ADD
    #define PATTERN_GETPC_PUSH2_ADD
    #define PATTERN_GETPC_PUSH1_ADD
    #define PATTERN_GETSP_PUSH1_ADD
    #define PATTERN_GETSP_PUSH2_ADD
    #define PATTERN_GETSP_PUSH1
    #define PATTERN_PUSH0
    #define PATTERN_PUSH1_POW2
    #define PATTERN_PUSH1_ALU
    #define PATTERN_PUSH1N
    #define PATTERN_PUSH1_HIGH4
    #define PATTERN_PUSH2
    #define PATTERN_PUSH4
    #define PATTERN_LT
    #define PATTERN_XOR
#endif


// define INSN to be used -> 0: remove; 1: use pattern, and; 2: recode
#ifdef RECODE_NATIVE_INSN
    #define NEW_NOP_INSN     2
    #define NEW_GET_PC_INSN  2
    #define NEW_GET_SP_INSN  2
    #define NEW_PUSH0_INSN   2
    #define NEW_PUSH1_INSN   2
    #define NEW_PUSH2_INSN   2
    #define NEW_PUSH4_INSN   2
    #define NEW_LT_INSN      2
    #define NEW_XOR_INSN     2
#endif
#ifdef PATTERN_NOPN
    #define NOP2_INSN        2
    #define NOP4_INSN        2
    #define NOP8_INSN        2
#endif
#ifdef PATTERN_GETPC_PUSH8_ADD
    #define LD1_PC_8_INSN    2
    #define LD2_PC_8_INSN    2
    #define LD4_PC_8_INSN    2
    #define LD8_PC_8_INSN    2
    #define ST1_PC_8_INSN    2
    #define ST2_PC_8_INSN    2
    #define ST4_PC_8_INSN    2
    #define ST8_PC_8_INSN    2
    #define PC_8_JUMP_INSN   2
    #define PC_8_INSN        2
#endif
#ifdef PATTERN_GETPC_PUSH4_ADD
    #define LD1_PC_4_INSN    2
    #define LD2_PC_4_INSN    2
    #define LD4_PC_4_INSN    2
    #define LD8_PC_4_INSN    2
    #define ST1_PC_4_INSN    2
    #define ST2_PC_4_INSN    2
    #define ST4_PC_4_INSN    2
    #define ST8_PC_4_INSN    2
    #define PC_4_JUMP_INSN   2
    #define PC_4_INSN        2
#endif
#ifdef PATTERN_GETPC_PUSH2_ADD
    #define LD1_PC_2_INSN    2
    #define LD2_PC_2_INSN    2
    #define LD4_PC_2_INSN    2
    #define LD8_PC_2_INSN    2
    #define ST1_PC_2_INSN    2
    #define ST2_PC_2_INSN    2
    #define ST4_PC_2_INSN    2
    #define ST8_PC_2_INSN    2
    #define PC_2_JUMP_INSN   2
    #define PC_2_INSN        2
#endif
#ifdef PATTERN_GETPC_PUSH1_ADD
    #define LD1_PC_1_INSN    2
    #define LD2_PC_1_INSN    2
    #define LD4_PC_1_INSN    2
    #define LD8_PC_1_INSN    2
    #define ST1_PC_1_INSN    2
    #define ST2_PC_1_INSN    2
    #define ST4_PC_1_INSN    2
    #define ST8_PC_1_INSN    2
    #define PC_1_JUMP_INSN   2
    #define PC_1_NOP_INSN    2
    #define PC_OFFSET_INSN   2
#endif
#ifdef PATTERN_GETSP_PUSH1_ADD
    #define LD1_SP_1_INSN    2
    #define LD2_SP_1_INSN    2
    #define LD4_SP_1_INSN    2
    #define LD8_SP_1_INSN    2
    #define ST1_SP_1_INSN    2
    #define ST2_SP_1_INSN    2
    #define ST4_SP_1_INSN    2
    #define ST8_SP_1_INSN    2
    #define CHANGE_SP_INSN   2
    #define SP_OFFSET_INSN   2
#endif
#ifdef PATTERN_GETSP_PUSH2_ADD
    #define LD1_SP_2_INSN    2
    #define LD2_SP_2_INSN    2
    #define LD4_SP_2_INSN    2
    #define LD8_SP_2_INSN    2
    #define ST1_SP_2_INSN    2
    #define ST2_SP_2_INSN    2
    #define ST4_SP_2_INSN    2
    #define ST8_SP_2_INSN    2
    #define SP_2_INSN        2
#endif
#ifdef PATTERN_GETSP_PUSH1
    #define DEC_SP_1_INSN    2
    #define SP_1_INSN        0
#endif
#ifdef PATTERN_PUSH0
    #define SHORT_JUMP_INSN  2
    #define XOR_0_INSN       2
    #define NOT_0_MUL_INSN   0
#endif
#ifdef PATTERN_PUSH1_ALU
    #define LT_1_JZ_INSN     2
    #define NOT_1_ADD_INSN   0
    #define LT_1_NOT_INSN    0
    #define LT_1_JNZ_INSN    0
#endif
#ifdef PATTERN_PUSH1_POW2
    #define POW2_1_ADD_INSN  2
    #define POW2_1_MUL_INSN  2
    #define POW2_1_LT_INSN   2
    #define POW2_1_DIV_INSN  2
    #define POW2_1_INSN      2
#endif
#ifdef PATTERN_PUSH1N
    #define PUSH1X2_INSN     2
    #define PUSH1X4_INSN     2
#endif
#ifdef PATTERN_PUSH1_HIGH4
    #define C1TOSTACK1_INSN  2
    #define C1TOSTACK2_INSN  2
    #define C1TOSTACK4_INSN  2
    #define C1TOSTACK8_INSN  2
    #define JUMP_PC_1_INSN   2
#endif
#ifdef PATTERN_PUSH2
    #define JUMP_PC_2_INSN   2
    #define C2TOSTACK1_INSN  0
    #define C2TOSTACK2_INSN  2
    #define C2TOSTACK4_INSN  2
    #define C2TOSTACK8_INSN  2
#endif
#ifdef PATTERN_PUSH4
    #define JUMP_PC_4_INSN   2
#endif
#ifdef PATTERN_LT
    #define LT_JZ_INSN       2
    #define LT_NOT_JZ_INSN   2
#endif
#ifdef PATTERN_XOR
    #define XOR_1_LT_INSN    2
#endif

// The PRECEDING is only available if OPTENABLED==1
////////////////////////////////////////////////////////////////////////////////

// insn in this gruop always exist (recoded or not)
#if (!defined(RECODE_INSN) || !defined(RECODE_NATIVE_INSN))
#undef NEW_NOP_INSN
#undef NEW_GET_PC_INSN
#undef NEW_GET_SP_INSN
#undef NEW_PUSH0_INSN
#undef NEW_PUSH1_INSN
#undef NEW_PUSH2_INSN
#undef NEW_PUSH4_INSN
#undef NEW_LT_INSN
#undef NEW_XOR_INSN
#define NEW_NOP_INSN         1
#define NEW_GET_PC_INSN      1
#define NEW_GET_SP_INSN      1
#define NEW_PUSH0_INSN       1
#define NEW_PUSH1_INSN       1
#define NEW_PUSH2_INSN       1
#define NEW_PUSH4_INSN       1
#define NEW_LT_INSN          1
#define NEW_XOR_INSN         1
#endif


// HEADERS

// include emulator header file after defines
#include "ivm_emu.h"
#if defined(WITH_IO)
#undef NO_IO
// IO instructions
#include "ivm_io.h"
#else
#define NO_IO
#define OUTPUT_PUTCHAR stderr
#define OUTPUT_OUTPUT stderr
#endif


// Check machine requeriments
#if (defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER==__BIG_ENDIAN))
#error Little-endian machine required.
#endif

#if (UINTPTR_MAX!=0xffffffffffffffff)
#error 64-bit machine required.
#endif

////////////////////////////////////////////////////////////////////////////////
//
//        //////////////////////////////////
//        //                              //
//        //    THE BODY OF THE EMULATOR  //
//        //                              //
//        //////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

// Default memory size in bytes
#define MEMBYTES (16UL*1024*1024)

// Output stream for error messages and putchar, respectively
// By default: stdout for program messages and stderr for put_char
#define OUTPUT_MSG stdout

// Global stuff
char *PC;               // Program counter
char *SP;               // Stack pointer
char *Mem;              // The memory
unsigned long MemBytes; // Size of the memory
void* addr[256];        // Where to go to execute the instruction

//Program bytes are placed from Mem[execStart] to Mem[execEnd],
//both included
unsigned long execStart = 0; // First position of the bytecode in memory
unsigned long execEnd= 0;    // Last position of the bytecode in memory
unsigned long argStart = 0;  // First position of the argument file in memory
unsigned long argEnd= 0;     // Last position of the argument file in memory

#if ((VERBOSE >= 2) || defined(HISTOGRAM))
typedef struct insn_attr {
    const char *name; // Name of the instruction
    int opbytes;      // Number of bytes of the immediate operand 
                      // As the opcode is 1-byte long, the total size 
                      // of the instruction is:
                      // opbytes + 1  
} insn_attr_t;
// Array of attributes for all the instruction set
insn_attr_t insn_attributes[256];
#endif

#ifdef HISTOGRAM
unsigned long histogram[256];
unsigned long histo2[256];
#endif


// Global options
char *opt_bycodefile = NULL;           // Binary bytecode file name
unsigned long opt_maxmem = MEMBYTES;   // Memory size (-m <number>)
char* argFile = NULL;
char* inpDir = NULL;
char* outDir = NULL;


/*
    Process command line options
*/
int get_options(int argc, char* argv[]) {
    int i, c;
    while ((c = getopt(argc, argv, "m:o:i:a:")) != -1) {
        switch (c) {
          case 'm': opt_maxmem = atol(optarg)>0?atol(optarg):opt_maxmem; break;
          case 'o': outDir = optarg; break;
          case 'i': inpDir = optarg; break;
          case 'a': argFile = optarg; break;
          case '?': // pass through
          default:
            if (optopt == 'm')
              fprintf(OUTPUT_MSG, "Option -%c requires an argument.\n", optopt);
            else
              fprintf(OUTPUT_MSG, "Unknown option `\\x%x'.\n", optopt);
            return 0;
        }
    }

    for (i = optind; i < argc; i++) {
        opt_bycodefile = argv[i];
        break;
    }

    if (!opt_bycodefile) {
        fprintf(OUTPUT_MSG, "Usage:\n\t%s [-m <size in bytes>] "
                            "[-o <output dir>] [-i <input dir>] "
                            "[-a <arg file>] <ivm binary file>\n", 
                argv[0]);
        return 0;
    }

    #if (VERBOSE>0)
        fprintf(OUTPUT_MSG, "opt_maxmem=%ld, opt_bycodefile='%s'\n",
                opt_maxmem, opt_bycodefile);
        if (outDir)
            fprintf(OUTPUT_MSG, "outDir=%s\n", outDir); 
        if (inpDir)
            fprintf(OUTPUT_MSG, "inpDir=%s\n", inpDir); 
        if (argFile)
            fprintf(OUTPUT_MSG, "argFile=%s\n", argFile); 
        fprintf(OUTPUT_MSG, "\n"); 
    #endif

    return 1;
}

/*
    Read a ivm binary bycode file and load it into memory 
    starting at position 'offset'
    (Mem[offset] is the first byte of the program)
*/
int ivm_read_bin(char *filename, unsigned long offset, unsigned long *m_start, unsigned long *m_end)
{
    FILE *fd;
    fd = fopen(filename, "r");
    if (!fd){
        fprintf(OUTPUT_MSG, "Can't open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    long fs; // File size
    fs = fread(&Mem[offset], 1, MemBytes-offset, fd); 
    
    #if (VERBOSE>0)
        fprintf(OUTPUT_MSG, "Read %ld bytes from '%s'\n", fs, filename);
    #endif

    if ( !feof(fd)){
        fprintf(OUTPUT_MSG, "Not enough memory to load '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    if (ferror(fd)){
        fprintf(OUTPUT_MSG, "Error reading '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    *m_start = offset;
    *m_end   = offset + fs -1;

    fclose(fd);
    return 0;
}


/*
    Print the last n elements of the stack 
    FORMAT_STACK_ROW: for tracing the stack, elements in a row
    FORMAT_STACK_IVM: to print at the end the resulting stack
                      like the ivm application
*/

// Address translation
char* idx2addr(unsigned long i);
unsigned long addr2idx(char* p);
inline char* idx2addr(unsigned long i){ return (char *)&(Mem[i]); }
inline unsigned long addr2idx(char* p){ return (unsigned long)p - (unsigned long)Mem; }

enum format_stack {FORMAT_STACK_ROW, FORMAT_STACK_IVM};
void print_stack(int format, unsigned int n){
    int interactive = isatty(fileno(OUTPUT_MSG));
    int i;

    // The first valid stack position is one word over 
    // MemBytes-BYTESPERWORD, so we need to substract twice
    char *stack_start = idx2addr(MemBytes - BYTESPERWORD - BYTESPERWORD); 

    // Start n positions over SP without going over the start of the stack
    char *p_start = MIN(stack_start, (char*)((uint64_t)(SP) + n*BYTESPERWORD));
    char *p;
    if (format == FORMAT_STACK_ROW) {
        fprintf(OUTPUT_MSG, "\tSTACK = [");
        for (p=p_start; p>=SP; p=(char*)(((WORD_T*)p)-1)){
            WORD_T val = *((WORD_T*)p);
            //if (val < 0xffffff)
                fprintf(OUTPUT_MSG, " | %#lx", val);
            //else
            //    fprintf(OUTPUT_MSG, " | 0x..%lx", val & 0xffffff));
        }
        fprintf(OUTPUT_MSG, "]\n");
    }

    if (format == FORMAT_STACK_IVM) {
        fprintf(OUTPUT_MSG, "End stack:\n");
        for (p=SP, i=1; p<=p_start; p=(char*)(((WORD_T*)p)+1), i++){
            WORD_T val = *((WORD_T*)p);
            fprintf(OUTPUT_MSG, "0x..%06lx %ld\n", val & 0xffffff, val);
            if (interactive && (i%1000 == 0)) {
                fprintf(OUTPUT_MSG, "--Press Enter--\n");
                getchar();
            }
        }
        fprintf(OUTPUT_MSG, "\n");
    }
}

#if (VERBOSE >= 3)
/*
    Dump the memory contents
*/
int ivm_mem_dump(unsigned long start, unsigned long end){
    unsigned long i,k;
    int rowbytes = 16; // bytes shown per row 

    fprintf(OUTPUT_MSG, "%#016lx\t", start);
    for (i = start; i <= end; i++){
        for (k = i; k <= MIN(i + rowbytes - 1, end); k++) {
            fprintf(OUTPUT_MSG, "%02x  ", (unsigned char)Mem[k]);
        }
        i = k-1;
        fprintf(OUTPUT_MSG, "\n");
        fprintf(OUTPUT_MSG, "%#016lx\t", k);
    }
    fprintf(OUTPUT_MSG, "\n");
}

/*
    Print the value of SP and the last n elements of the stack
*/
void print_stack_status(){
    fprintf(OUTPUT_MSG, "\tSP = %p\n", SP);
    fprintf(OUTPUT_MSG, "\tTOS = %#lx\n", (*(WORD_T*)SP) );
    print_stack(FORMAT_STACK_ROW, 16);
}
#endif

#if (VERBOSE >= 2)
void print_insn(char *pc){
    int opbytes;
    unsigned char op_code = *pc;
    fprintf(OUTPUT_MSG, "PC=%p Mem[%ld] -> op_code=%#02x %s\t",
                     pc, addr2idx(pc), op_code, insn_attributes[op_code].name);
    opbytes = insn_attributes[op_code].opbytes;

    uint8_t  next1;
    uint16_t next2;
    uint32_t next4;
    uint64_t next8;
    switch(opbytes){
        case 1:
            next1 = *((uint8_t*)((uint64_t)pc+1));
            fprintf(OUTPUT_MSG, "oper(1byte)=%#x", next1);
            break;
        case 2:
            next2 = *((uint16_t*)((uint64_t)pc+1));
            fprintf(OUTPUT_MSG, "oper(2bytes)=%#x", next2);
            break;
        case 4:
            next4 = *((uint32_t*)((uint64_t)pc+1));
            fprintf(OUTPUT_MSG, "oper(4bytes)=%#x", next4);
            break;
        case 8:
            next8 = *((uint64_t*)((uint64_t)pc+1));
            fprintf(OUTPUT_MSG, "oper(4bytes)=%#lx", next8);
            break;
    }
    fprintf(OUTPUT_MSG, "\n");
}
#endif


// setjmp/longjmp stuf
jmp_buf env;
void signal_handler(int s)
{
    longjmp(env,s); // return the signal number
}


// Stack operations
void push(WORD_T v);
WORD_T pop();
inline void push(WORD_T v){ SP-=BYTESPERWORD; *((WORD_T*)SP)=(WORD_T)v; }
inline WORD_T pop(){ WORD_T v=*((WORD_T*)SP); SP+=BYTESPERWORD; return v; }


int main(int argc, char* argv[]){
    int error = 0; // Any error that stops simulation
    uint8_t opcode1;
    #ifndef NOOPT
    uint32_t opcode4;  
    uint32_t high4;
    uint64_t opcode8;
    uint64_t PCplus;
    uint64_t SPplus;
    uint8_t next1_val;
    uint8_t next1_addr;
    uint16_t next2_val;
    uint16_t next2_addr;
    #endif

    // Instruction operand (unsigned)
    uint8_t  next1;
    uint16_t next2;
    uint32_t next4;
    uint64_t next8;


    
    // Instruction operand (signed, only for jump offset)
    int8_t  next1s;
    int64_t offset; // offset for jump_zero (signed)

    uint64_t a, b, r, u, v; // Unsigned aux. variables
    int64_t  x, y;          // Signed aux. variables 

    long segment_start = 0; // Where to load the bytecode
                            // Mem[segment_start] is the
                            // first byte of the program 

    char *filename;

    fprintf(OUTPUT_MSG, "Yet another ivm emulator, %s\n", VERSION);
    #if (VERBOSE>0)
        fprintf(OUTPUT_MSG, "VERBOSE=%d compilation\n",VERBOSE);
    #endif
    #ifdef NOOPT
        fprintf(OUTPUT_MSG, "NOOPT compilation\n");
    #endif
    #ifdef STEPCOUNT
        fprintf(OUTPUT_MSG, "STEPCOUNT compilation\n");
    #endif
    #ifdef HISTOGRAM
        fprintf(OUTPUT_MSG, "HISTOGRAM compilation\n");
    #endif
    fprintf(OUTPUT_MSG,"\n");

    if (!get_options(argc, argv)){
        exit(EXIT_FAILURE);
    }

    // Get options
    filename = opt_bycodefile; 
    MemBytes = opt_maxmem;

    // Prepare the memory
    Mem = (char*)malloc(MemBytes * sizeof(char));

    #ifndef NO_IO
    ioInitIn();
    ioInitOut();
    #endif

    // Instruction Set.
    init_insn_addr(addr);
	#if ((VERBOSE >= 2) || defined(HISTOGRAM))
    init_insn_attributes(insn_attributes);
	#endif
        
    // Read bytecode file
    ivm_read_bin(filename, segment_start, &execStart, &execEnd);
    #if (VERBOSE>0)
    fprintf(OUTPUT_MSG, "First byte of the program indexed by %#lx (=%ld), last byte by %#lx (=%ld)\n", 
            execStart, execStart, execEnd, execEnd);
    fprintf(OUTPUT_MSG, "\n");
    #endif

    *(uint64_t*)&Mem[execEnd+1]=0;

    // Read argument file
    if (argFile){
        ivm_read_bin(argFile, execEnd+9, &argStart, &argEnd);
        *(uint64_t*)(&Mem[execEnd+1]) = argEnd - argStart + 1;
        #if (VERBOSE>0)
        fprintf(OUTPUT_MSG, "First byte of the argument file indexed by %#lx (=%ld), last byte by %#lx (=%ld)\n", 
                argStart, argStart, argEnd, argEnd);
        fprintf(OUTPUT_MSG, "\n");
        #endif
    }

    #if (VERBOSE >= 3)
    if (execEnd - execStart < 1024*100) { 
       // Dump memory only for small programs
        ivm_mem_dump(execStart, execEnd);
    }
    #endif 

    PC = idx2addr(segment_start);  // =execStartPC
    SP = idx2addr(MemBytes - BYTESPERWORD); 

    #if (VERBOSE == 2)
        #define VERBOSE_ACTION do{ print_insn(PC-1);} while(0) 
    #elif (VERBOSE >= 3)
        #define VERBOSE_ACTION do{ print_insn(PC-1); print_stack_status();} while(0) 
    #else
        #define VERBOSE_ACTION 
    #endif

    #ifdef HISTOGRAM
        #define HISTOGRAM_ACTION(op)    do{histogram[op]++;}while(0)
        #define HISTOGRAM_UNDO(op)      do{histogram[op]--;}while(0)
        #define HISTOGRAM_RECODE(op)    do{histo2[op]++;}while(0)
        #undef STEPCOUNT
        #define STEPCOUNT
    #else
        #define HISTOGRAM_ACTION(op)
        #define HISTOGRAM_UNDO(op)
        #define HISTOGRAM_RECODE(op)
    #endif

    #ifdef STEPCOUNT
        unsigned long steps = 0, fetchs = 0; // Instruction/fetch count
        #define STEPCOUNT_ACTION(n)  do{steps+=n;}while(0)
        #define FETCHCOUNT_ACTION    do{fetchs++;}while(0)
    #else 
        #define STEPCOUNT_ACTION(n)
        #define FETCHCOUNT_ACTION
    #endif

    #ifdef RECODE_INSN
    #define MODIF2(X)   HISTOGRAM_UNDO(opcode1);         \
                        HISTOGRAM_RECODE(OPCODE_##X);    \
                        HISTOGRAM_ACTION(OPCODE_##X);    \
                        *(uint8_t *)(PC-1)=OPCODE_##X; X:
    #else // use (existing) patterns but no recode insn
    #define MODIF2(X)   HISTOGRAM_UNDO(opcode1);         \
                        HISTOGRAM_ACTION(OPCODE_##X)
    #endif
    #define MODIF1(X)
    #define MODIF0(X)
    #define RECODE(X)   concat(MODIF,X##_INSN)(X)

    #ifdef NOOPT
    #define FETCH   opcode1=*(uint8_t*)PC; PC++;         \
                    FETCHCOUNT_ACTION;                   \
                    HISTOGRAM_ACTION(opcode1)
    #else
    #define FETCH   opcode1=*(uint8_t*)PC; PC++;         \
                    if (opcode1 <= OPCODE_PUSH4) {       \
                        opcode4 = *(uint32_t*)(PC-1);    \
                    } else if (opcode1 == OPCODE_LT) {   \
                        opcode4 = *(uint32_t*)(PC-1);    \
                    } else if (opcode1 >= OPCODE_XOR) {  \
                        opcode4 = *(uint32_t*)(PC-1);    \
                    }                                    \
                    FETCHCOUNT_ACTION;                   \
                    HISTOGRAM_ACTION(opcode1)
    #endif

    #define EXEC    STEPCOUNT_ACTION(1);                 \
                    VERBOSE_ACTION;                      \
                    goto *addr[opcode1]

    #define NEXT    FETCH; EXEC

    error=setjmp(env);
    if (error == 0) {
        signal(SIGINT,signal_handler);
        signal(SIGSEGV,signal_handler);
        signal(SIGFPE,signal_handler);
        NEXT;
    }

    //-----------------
    EXIT:
        goto HALT;
    //-----------------
    NOP: 
    #if OPTENABLED
        #ifdef PATTERN_NOPN
            #if (NOP4_INSN > 0 || NOP8_INSN > 0)
            if (opcode4 == 0x01010101) {
                #if (NOP8_INSN > 0)
                if (*(uint32_t*)(PC+3) == 0x01010101) {
                    RECODE(NOP8);    // NOP/NOP/NOP/NOP/NOP/NOP/NOP/NOP
                    PC+=7; STEPCOUNT_ACTION(7); NEXT;
                } else
                #endif
                #if (NOP4_INSN > 0)
                {
                    RECODE(NOP4);    // NOP/NOP/NOP/NOP
                    PC+=3; STEPCOUNT_ACTION(3); NEXT;
                }
                #else
                {
                    NEXT;
                }
                #endif
            } else
            #endif
            #if (NOP2_INSN > 0)
            if (opcode4 == 0x0101) {
                RECODE(NOP2);    // NOP/NOP
                PC+=1; STEPCOUNT_ACTION(1); NEXT;
            } else
            #endif
        #endif
        {
            RECODE(NEW_NOP);
            NEXT;
        }
    #endif
        NEXT;
    //-----------------
    JUMP:
        a = pop();
        PC = (char*)a;
        NEXT;
    //-----------------
    JUMP_ZERO:
        next1s = *((int8_t*)PC);
        PC+=1;
        offset = next1s;
        a = pop();
        if (a == 0){
            PC = (char*)((uint64_t)PC + offset); 
        }
        NEXT;
    //-----------------
    SET_SP:
        SP = (char*)*((WORD_T*)SP);
        NEXT;
    //-----------------
    GET_PC:
    #if OPTENABLED
        #ifdef PATTERN_GETPC_PUSH1_ADD
        if (0x020000800 == (opcode4 & 0x0ff00ff00)){
            switch (*(uint8_t*)(PC+3)) {
                #if (PC_1_NOP_INSN > 0)
                case OPCODE_NOP:
                    RECODE(PC_1_NOP);    // GET_PC/PUSH1/ADD/NOP
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    push(PCplus);
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD1_PC_1_INSN > 0)
                case OPCODE_LOAD1:
                    RECODE(LD1_PC_1);    // GET_PC/PUSH1/ADD/LOAD1
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    push(*((uint8_t  *)(PCplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD2_PC_1_INSN > 0)
                case OPCODE_LOAD2:
                    RECODE(LD2_PC_1);    // GET_PC/PUSH1/ADD/LOAD2
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    push(*((uint16_t *)(PCplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD4_PC_1_INSN > 0)
                case OPCODE_LOAD4:
                    RECODE(LD4_PC_1);    // GET_PC/PUSH1/ADD/LOAD4
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    push(*((uint32_t *)(PCplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD8_PC_1_INSN > 0)
                case OPCODE_LOAD8:
                    RECODE(LD8_PC_1);    // GET_PC/PUSH1/ADD/LOAD8
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    push(*((uint64_t *)(PCplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST1_PC_1_INSN > 0)
                case OPCODE_STORE1:
                    RECODE(ST1_PC_1);    // GET_PC/PUSH1/ADD/STORE1
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    *((uint8_t *) PCplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST2_PC_1_INSN > 0)
                case OPCODE_STORE2:
                    RECODE(ST2_PC_1);    // GET_PC/PUSH1/ADD/STORE2
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    *((uint16_t*) PCplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST4_PC_1_INSN > 0)
                case OPCODE_STORE4:
                    RECODE(ST4_PC_1);    // GET_PC/PUSH1/ADD/STORE4
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    *((uint32_t*) PCplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST8_PC_1_INSN > 0)
                case OPCODE_STORE8:
                    RECODE(ST8_PC_1);    // GET_PC/PUSH1/ADD/STORE8
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    *((uint64_t*) PCplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (PC_1_JUMP_INSN > 0)
                case OPCODE_JUMP:
                    RECODE(PC_1_JUMP);    // GET_PC/PUSH1/ADD/JUMP
                    next1 = opcode4 >> 16;
                    PC += (WORD_T)next1;
                    STEPCOUNT_ACTION(3); NEXT;
                #endif
                default:
                #if (PC_OFFSET_INSN > 0)
                    RECODE(PC_OFFSET);    // GET_PC/PUSH1/ADD
                    next1 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next1;
                    push(PCplus);
                    PC+=3; STEPCOUNT_ACTION(2); NEXT;
                #endif
                // backstop for default (get_pc)
                    push((WORD_T)PC);
                    NEXT;
            }
        } else
        #endif
        #ifdef PATTERN_GETPC_PUSH2_ADD
        if (0x0900 == (opcode4 & 0x0ff00)){
            switch (*(uint16_t*)(PC+3)) {
                #if (LD1_PC_2_INSN > 0)
                case (OPCODE_LOAD1<<8)|OPCODE_ADD:
                    RECODE(LD1_PC_2);    // GET_PC/PUSH2/ADD/LOAD1
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    push(*((uint8_t  *)(PCplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD2_PC_2_INSN > 0)
                case (OPCODE_LOAD2<<8)|OPCODE_ADD:
                    RECODE(LD2_PC_2);    // GET_PC/PUSH2/ADD/LOAD2
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    push(*((uint16_t *)(PCplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD4_PC_2_INSN > 0)
                case (OPCODE_LOAD4<<8)|OPCODE_ADD:
                    RECODE(LD4_PC_2);    // GET_PC/PUSH2/ADD/LOAD4
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    push(*((uint32_t *)(PCplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD8_PC_2_INSN > 0)
                case (OPCODE_LOAD8<<8)|OPCODE_ADD:
                    RECODE(LD8_PC_2);    // GET_PC/PUSH2/ADD/LOAD8
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    push(*((uint64_t *)(PCplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST1_PC_2_INSN > 0)
                case (OPCODE_STORE1<<8)|OPCODE_ADD:
                    RECODE(ST1_PC_2);    // GET_PC/PUSH2/ADD/STORE1
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    *((uint8_t *) PCplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST2_PC_2_INSN > 0)
                case (OPCODE_STORE2<<8)|OPCODE_ADD:
                    RECODE(ST2_PC_2);    // GET_PC/PUSH2/ADD/STORE2
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    *((uint16_t*) PCplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST4_PC_2_INSN > 0)
                case (OPCODE_STORE4<<8)|OPCODE_ADD:
                    RECODE(ST4_PC_2);    // GET_PC/PUSH2/ADD/STORE4
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    *((uint32_t*) PCplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST8_PC_2_INSN > 0)
                case (OPCODE_STORE8<<8)|OPCODE_ADD:
                    RECODE(ST8_PC_2);    // GET_PC/PUSH2/ADD/STORE8
                    next2 = opcode4 >> 16;
                    PCplus = (WORD_T)PC + (WORD_T)next2;
                    *((uint64_t*) PCplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (PC_2_JUMP_INSN > 0)
                case (OPCODE_JUMP<<8)|OPCODE_ADD:
                    RECODE(PC_2_JUMP);    // GET_PC/PUSH2/ADD/JUMP
                    next2 = opcode4 >> 16;
                    PC += (WORD_T)next2;
                    STEPCOUNT_ACTION(3); NEXT;
                #endif
                default:    // GET_PC/PUSH2
                #if (PC_2_INSN > 0)
                    RECODE(PC_2);    //get_pc/push2
                    push((WORD_T)PC);
                    next2 = opcode4 >> 16;
                    push((WORD_T)next2);
                    PC+=3; STEPCOUNT_ACTION(1); NEXT;
                #endif
                // backstop for default (get_pc)
                    push((WORD_T)PC);
                    NEXT;
            }
        } else
        #endif
        #ifdef PATTERN_GETPC_PUSH4_ADD
        if (0x0a00==(opcode4 & 0x0ff00)) {
            switch (*(uint16_t*)(PC+5)) {
                #if (LD1_PC_4_INSN > 0)
                case (OPCODE_LOAD1<<8)|OPCODE_ADD:
                    RECODE(LD1_PC_4);    // GET_PC/PUSH4/ADD/LOAD1
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    push(*((uint8_t  *)(PCplus)));
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD2_PC_4_INSN > 0)
                case (OPCODE_LOAD2<<8)|OPCODE_ADD:
                    RECODE(LD2_PC_4);    // GET_PC/PUSH4/ADD/LOAD2
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    push(*((uint16_t *)(PCplus)));
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD4_PC_4_INSN > 0)
                case (OPCODE_LOAD4<<8)|OPCODE_ADD:
                    RECODE(LD4_PC_4);    // GET_PC/PUSH4/ADD/LOAD4
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    push(*((uint32_t *)(PCplus)));
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD8_PC_4_INSN > 0)
                case (OPCODE_LOAD8<<8)|OPCODE_ADD:
                    RECODE(LD8_PC_4);    // GET_PC/PUSH4/ADD/LOAD8
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    push(*((uint64_t *)(PCplus)));
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST1_PC_4_INSN > 0)
                case (OPCODE_STORE1<<8)|OPCODE_ADD:
                    RECODE(ST1_PC_4);    // GET_PC/PUSH4/ADD/STORE1
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    *((uint8_t  *)(PCplus)) = pop();
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST2_PC_4_INSN > 0)
                case (OPCODE_STORE2<<8)|OPCODE_ADD:
                    RECODE(ST2_PC_4);    // GET_PC/PUSH4/ADD/STORE2
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    *((uint16_t *)(PCplus)) = pop();
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST4_PC_4_INSN > 0)
                case (OPCODE_STORE4<<8)|OPCODE_ADD:
                    RECODE(ST4_PC_4);    // GET_PC/PUSH4/ADD/STORE4
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    *((uint32_t *)(PCplus)) = pop();
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST8_PC_4_INSN > 0)
                case (OPCODE_STORE8<<8)|OPCODE_ADD:
                    RECODE(ST8_PC_4);    // GET_PC/PUSH4/ADD/STORE8
                    next4 = *(uint32_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next4;
                    *((uint64_t *)(PCplus)) = pop();
                    PC+=7; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (PC_4_JUMP_INSN > 0)
                case (OPCODE_JUMP<<8)|OPCODE_ADD:
                    RECODE(PC_4_JUMP);    // GET_PC/PUSH4/ADD/JUMP
                    next4 = *(uint32_t*)(PC+1);
                    PC += (WORD_T)next4;
                    STEPCOUNT_ACTION(3); NEXT;
                #endif
                default: // GET_PC/PUSH4
                #if (PC_4_INSN > 0)
                    RECODE(PC_4);    // get_pc/push4
                    push((WORD_T)PC);
                    next4 = *(uint32_t*)(PC+1);
                    push((WORD_T)next4);
                    PC+=5; STEPCOUNT_ACTION(1); NEXT;
                #endif
                // backstop for default (get_pc)
                    push((WORD_T)PC);
                    NEXT;
            }
        } else
        #endif
        #ifdef PATTERN_GETPC_PUSH8_ADD
        if (0x0b00==(opcode4 & 0x0ff00)) {
            switch (*(uint16_t*)(PC+9)) {
                #if (LD1_PC_8_INSN > 0)
                case (OPCODE_LOAD1<<8)|OPCODE_ADD:
                    RECODE(LD1_PC_8);    // GET_PC/PUSH8/ADD/LOAD1
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    push(*((uint8_t  *)(PCplus)));
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD2_PC_8_INSN > 0)
                case (OPCODE_LOAD2<<8)|OPCODE_ADD:
                    RECODE(LD2_PC_8);    // GET_PC/PUSH8/ADD/LOAD2
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    push(*((uint16_t *)(PCplus)));
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD4_PC_8_INSN > 0)
                case (OPCODE_LOAD4<<8)|OPCODE_ADD:
                    RECODE(LD4_PC_8);    // GET_PC/PUSH8/ADD/LOAD4
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    push(*((uint32_t *)(PCplus)));
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD8_PC_8_INSN > 0)
                case (OPCODE_LOAD8<<8)|OPCODE_ADD:
                    RECODE(LD8_PC_8);    // GET_PC/PUSH8/ADD/LOAD8
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    push(*((uint64_t *)(PCplus)));
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST1_PC_8_INSN > 0)
                case (OPCODE_STORE1<<8)|OPCODE_ADD:
                    RECODE(ST1_PC_8);    // GET_PC/PUSH8/ADD/STORE1
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    *((uint8_t  *)(PCplus)) = pop();
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST2_PC_8_INSN > 0)
                case (OPCODE_STORE2<<8)|OPCODE_ADD:
                    RECODE(ST2_PC_8);    // GET_PC/PUSH8/ADD/STORE2
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    *((uint16_t *)(PCplus)) = pop();
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST4_PC_8_INSN > 0)
                case (OPCODE_STORE4<<8)|OPCODE_ADD:
                    RECODE(ST4_PC_8);    // GET_PC/PUSH8/ADD/STORE4
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    *((uint32_t *)(PCplus)) = pop();
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST8_PC_8_INSN > 0)
                case (OPCODE_STORE8<<8)|OPCODE_ADD:
                    RECODE(ST8_PC_8);    // GET_PC/PUSH8/ADD/STORE8
                    next8 = *(uint64_t*)(PC+1);
                    PCplus = (WORD_T)PC + (WORD_T)next8;
                    *((uint64_t *)(PCplus)) = pop();
                    PC+=11; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (PC_8_JUMP_INSN > 0)
                case (OPCODE_JUMP<<8)|OPCODE_ADD:
                    RECODE(PC_8_JUMP);    // GET_PC/PUSH8/ADD/JUMP
                    next8 = *(uint64_t*)(PC+1);
                    PC += (WORD_T)next8;
                    STEPCOUNT_ACTION(3); NEXT;
                #endif
                default: // GET_PC/PUSH8
                #if (PC_8_INSN > 0)
                    RECODE(PC_8);    //get_pc/push8
                    push((WORD_T)PC);
                    next8 = *(uint64_t*)(PC+1);
                    push((WORD_T)next8);
                    PC+=9; STEPCOUNT_ACTION(1); NEXT;
                #endif
                // backstop for default (get_pc)
                    push((WORD_T)PC);
                    NEXT;
            }
        } else
        #endif
        {
            RECODE(NEW_GET_PC);
            push((WORD_T)PC);
            NEXT;
        }
    #else
        push((WORD_T)PC);
        NEXT;
    #endif
    //-----------------
    GET_SP:
    #if OPTENABLED
        #ifdef PATTERN_GETSP_PUSH1_ADD
        if (0x020000800 == (opcode4 & 0x0ff00ff00)) {
            switch (*(uint8_t*)(PC+3)) {
                #if (ST8_SP_1_INSN > 0)
                case OPCODE_STORE8:
                    RECODE(ST8_SP_1);    // get_sp/push1/add/store8
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    *((uint64_t*) SPplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD8_SP_1_INSN > 0)
                case OPCODE_LOAD8:
                    RECODE(LD8_SP_1);    // get_sp/push1/add/load8
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    push(*((uint64_t *)(SPplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD4_SP_1_INSN > 0)
                case OPCODE_LOAD4:
                    RECODE(LD4_SP_1);    // get_sp/push1/add/load4
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    push(*((uint32_t *)(SPplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST4_SP_1_INSN > 0)
                case OPCODE_STORE4:
                    RECODE(ST4_SP_1);    // get_sp/push1/add/store4
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    *((uint32_t*) SPplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD1_SP_1_INSN > 0)
                case OPCODE_LOAD1:
                    RECODE(LD1_SP_1);    // get_sp/push1/add/load1
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    push(*((uint8_t  *)(SPplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST1_SP_1_INSN > 0)
                case OPCODE_STORE1:
                    RECODE(ST1_SP_1);    // get_sp/push1/add/store1
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    *((uint8_t *) SPplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD2_SP_1_INSN > 0)
                case OPCODE_LOAD2:
                    RECODE(LD2_SP_1);    // get_sp/push1/add/load2
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    push(*((uint16_t *)(SPplus)));
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST2_SP_1_INSN > 0)
                case OPCODE_STORE2:
                    RECODE(ST2_SP_1);    // get_sp/push1/add/store2
                    next1 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next1;
                    *((uint16_t*) SPplus) = pop();
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (CHANGE_SP_INSN > 0)
                case OPCODE_SET_SP:
                    RECODE(CHANGE_SP);    // GET_SP/PUSH1/ADD/SET_SP
                    next1 = opcode4 >> 16;
                    SP = SP + next1;
                    PC+=4; STEPCOUNT_ACTION(3); NEXT;
                #endif
                default: // any other insn
                #if (SP_OFFSET_INSN > 0)
                    RECODE(SP_OFFSET);    // GET_SP/PUSH1/ADD
                    next1 = opcode4 >> 16;
                    push((WORD_T)SP+next1);
                    PC+=3; STEPCOUNT_ACTION(2); NEXT;
                #endif
                // backstop for default (get_sp)
                    push((WORD_T)SP);
                    NEXT;
            }    
        } else
        #endif
        #ifdef PATTERN_GETSP_PUSH1
        #if (DEC_SP_1_INSN > 0)
        if ((0x2a000800 == (opcode4 & 0x0ff00ff00)) &&
            (*(uint16_t*)(PC+3)==(OPCODE_SET_SP<<8|OPCODE_ADD))) {
            RECODE(DEC_SP_1);
            next1 = opcode4 >> 16;
            u = next1;
            SP = SP + ~u;
            PC+=5; STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (SP_1_INSN > 0)
        if (0x0800 == (opcode4 & 0x0ff00)) { // not followed by add
            RECODE(SP_1);    // get_sp/push1
            push((WORD_T)SP);
            next1 = opcode4 >> 16;
            push(next1);
            PC+=2; STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #endif
        #ifdef PATTERN_GETSP_PUSH2_ADD
        if (0x0900 == (opcode4 & 0x0ff00)) {
            switch (*(uint16_t*)(PC+3)) {
                #if (ST8_SP_2_INSN > 0)
                case (OPCODE_STORE8<<8)|OPCODE_ADD:
                    RECODE(ST8_SP_2);    // get_sp/push2/add/store8
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    *((uint64_t*) SPplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD8_SP_2_INSN > 0)
                case (OPCODE_LOAD8<<8)|OPCODE_ADD:
                    RECODE(LD8_SP_2);    // get_sp/push2/add/load8
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    push(*((uint64_t *)(SPplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD4_SP_2_INSN > 0)
                case (OPCODE_LOAD4<<8)|OPCODE_ADD:
                    RECODE(LD4_SP_2);    // get_sp/push2/add/load4
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    push(*((uint32_t *)(SPplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST4_SP_2_INSN > 0)
                case (OPCODE_STORE4<<8)|OPCODE_ADD:
                    RECODE(ST4_SP_2);    // get_sp/push2/add/store4
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    *((uint32_t*) SPplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD1_SP_2_INSN > 0)
                case (OPCODE_LOAD1<<8)|OPCODE_ADD:
                    RECODE(LD1_SP_2);    // get_sp/push2/add/load1
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    push(*((uint8_t  *)(SPplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (LD2_SP_2_INSN > 0)
                case (OPCODE_LOAD2<<8)|OPCODE_ADD:
                    RECODE(LD2_SP_2);    // get_sp/push2/add/load2
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    push(*((uint16_t *)(SPplus)));
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST1_SP_2_INSN > 0)
                case (OPCODE_STORE1<<8)|OPCODE_ADD:
                    RECODE(ST1_SP_2);    // get_sp/push2/add/store1
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    *((uint8_t *) SPplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                #if (ST2_SP_2_INSN > 0)
                case (OPCODE_STORE2<<8)|OPCODE_ADD:
                    RECODE(ST2_SP_2);    // get_sp/push2/add/store2
                    next2 = opcode4 >> 16;
                    SPplus = (WORD_T)SP +(WORD_T)next2;
                    *((uint16_t*) SPplus) = pop();
                    PC+=5; STEPCOUNT_ACTION(3); NEXT;
                #endif
                default:
                #if (SP_2_INSN > 0)
                    RECODE(SP_2);    // get_sp/push2/
                    push((WORD_T)SP);
                    next2 = opcode4 >> 16;
                    push(next2);
                    PC+=3; STEPCOUNT_ACTION(1); NEXT;
                #endif
                // backstop for default (get_sp)
                    push((WORD_T)SP);
                    NEXT;
            }
        } else
        #endif
        { // get_sp followed by any other insn (neither push1 nor push2)
            RECODE(NEW_GET_SP); // GET_SP
            push((WORD_T)SP);
            NEXT;
        }
    #else
        push((WORD_T)SP);
        NEXT;
    #endif        
    //-----------------
    PUSH0:
    #if OPTENABLED
        #ifdef PATTERN_PUSH0
        #if (SHORT_JUMP_INSN > 0)
        if (0x00300 == (opcode4 & 0x0ff00)){
            RECODE(SHORT_JUMP);    // PUSH0/JUMP_ZERO
            next1s = opcode4 >> 16; 
            PC = (char*)((uint64_t)PC + (int64_t)next1s + 2);
            STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #if (XOR_0_INSN > 0)
        if (0x002b00 == (opcode4 & 0x00ff00)) { // PUSH0/XOR = NOP
            RECODE(XOR_0);    // push0/xor
            PC++; STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #if (NOT_0_MUL_INSN > 0)
        if (0x212a00 == (opcode4 & 0x0ffff00)) { // *(-1)
            RECODE(NOT_0_MUL);    // push0/not/mul
            u=pop();
            push(~u+1);
            PC+=2; STEPCOUNT_ACTION(2); NEXT;
        } else
        #endif
        #endif
        {    // PUSH0
            RECODE(NEW_PUSH0);
            push(0);
            NEXT;
        }
    #else
        push(0);
        NEXT;
    #endif
    //-----------------
    PUSH1:
    #if OPTENABLED
        #ifdef PATTERN_PUSH1_POW2
        if (0x2c0000 == (opcode4 & 0x0ff0000)){
            uint32_t nextopcode = (opcode4 & 0x0ff000000);
            switch (nextopcode) {
                #if (POW2_1_ADD_INSN > 0)
                case OPCODE_ADD<<24:
                    RECODE(POW2_1_ADD);    // PUSH1/POW2/ADD
                    next1 = opcode4 >> 8;
                    x = (1UL << next1);                
                    y = pop();
                    push(x + y);
                    PC+=3; STEPCOUNT_ACTION(2); NEXT;
                #endif
                #if (POW2_1_DIV_INSN > 0)
                case OPCODE_DIV<<24:
                    RECODE(POW2_1_DIV);    // PUSH1/POW2/DIV
                    next1 = opcode4 >> 8; 
                    u = (1UL << next1);                
                    v = pop();
                    push(v / u);
                    PC+=3; STEPCOUNT_ACTION(2); NEXT;
                #endif
                #if (POW2_1_MUL_INSN > 0)
                case OPCODE_MUL<<24:
                    RECODE(POW2_1_MUL);    // PUSH1/POW2/MUL
                    next1 = opcode4 >> 8; 
                    x = (1UL << next1);                
                    y = pop();
                    push(x * y);
                    PC+=3; STEPCOUNT_ACTION(2); NEXT;
                #endif
                #if (POW2_1_LT_INSN > 0)
                case OPCODE_LT<<24:
                    RECODE(POW2_1_LT);    // PUSH1/POW2/LT
                    next1 = opcode4 >> 8; 
                    u = (1UL << next1);                
                    v = pop();
                    PC+=3; STEPCOUNT_ACTION(2);        
                    if (v < u) {
                        push(-1);
                        NEXT;
                    } else {
                        push(0);
                        NEXT;
                    }
                #endif
                default:    // Default case, none of the arithmetics above
                #if (POW2_1_INSN > 0)
                    RECODE(POW2_1);    // PUSH1/POW2 
                    next1 = opcode4 >> 8; 
                    u = (1UL << next1);                
                    push(u);
                    PC+=2;
                    STEPCOUNT_ACTION(1);
                    NEXT;
                #endif
                // backstop for default (push1)
                    next1 = *((uint8_t*)PC);
                    push((WORD_T)next1);
                    PC+=1; NEXT;
            }
        } else
        #endif
        #ifdef PATTERN_PUSH1_ALU
        #if (LT_1_JZ_INSN > 0)
        if (0x03240000 == (opcode4 & 0x0ffff0000)){
            RECODE(LT_1_JZ);    // PUSH1/LT/JUMP_ZERO
            next1 = opcode4 >> 8; 
            u = next1; 
            v = pop();
            STEPCOUNT_ACTION(2);
            if (v < u) {
                PC+=4; NEXT;
            } else {
                next1s =*(PC+3);
                PC = (char*)((uint64_t)PC + (int64_t)next1s + 4);
                NEXT;
            }
        } else
        #endif
        #if ((LT_1_NOT_INSN > 0) || (LT_1_JNZ_INSN > 0))
        if (0x2a240000 == (opcode4 & 0x0ffff0000)){
            #if (LT_1_JNZ_INSN > 0)
            if (*(uint8_t*)(PC+3) == OPCODE_JUMP_ZERO) {
                RECODE(LT_1_JNZ);    // PUSH1/LT/NOT/JUMP_ZERO
                next1 = opcode4 >> 8; 
                u = next1; 
                v = pop();
                STEPCOUNT_ACTION(2);
                if (v >= u) {
                    PC+=5; NEXT;
                } else {
                    next1s =*(PC+4);
                    PC = (char*)((uint64_t)PC + (int64_t)next1s + 5);
                    NEXT;
                }
            } else
            #endif
            #if (LT_1_NOT_INSN > 0)
            {
                RECODE(LT_1_NOT);    // PUSH1/LT/NOT
                next1 = opcode4 >> 8; 
                u = next1; 
                v = pop();
                STEPCOUNT_ACTION(2);
                if (v < u) {
                    push(0);
                } else {
                    push(-1);
                }
                PC+=3; STEPCOUNT_ACTION(2); NEXT;
            }
            #else
            { }
            #endif
        } else
        #endif
        #if (NOT_1_ADD_INSN > 0)
        if (0x202a0000 == (opcode4 & 0x0ffff0000)){
            RECODE(NOT_1_ADD);    // PUSH1/NOT/ADD
            next1 = opcode4 >> 8;
            u = next1;
            v = pop();
            push( v + ~u);
            PC+=3; STEPCOUNT_ACTION(2); NEXT;
        } else
        #endif
        #endif
        #ifdef PATTERN_PUSH1N
        #if (PUSH1X4_INSN > 0 || PUSH1X2_INSN > 0)
        if (0x00080000 == (opcode4 & 0x000ff0000)) {
            high4=*(uint32_t*)(PC+3);
            #if (PUSH1X4_INSN > 0)
            if (0x00080008 == (high4 & 0x000ff00ff)) {
                RECODE(PUSH1X4); // PUSH1/PUSH1/PUSH1/PUSH1
                next1 = opcode4 >> 8;
                push((WORD_T)next1);
                next1 = opcode4 >> 24;
                push((WORD_T)next1);
                next1 = high4 >> 8;
                push((WORD_T)next1);
                next1 = high4 >> 24;
                push((WORD_T)next1);
                PC+=7;
                STEPCOUNT_ACTION(3);
                for (opcode4 = *(uint32_t*)PC;
                    (opcode4 & 0x000ff00ff)==0x00080008;
                    PC+=4, opcode4 = *(uint32_t*)PC) {
                    push((WORD_T)*(uint8_t*)(PC+1));
                    push((WORD_T)*(uint8_t*)(PC+3));
                    STEPCOUNT_ACTION(2);
                }
                // opcode4 prefetched
                opcode1=*(uint8_t*)(PC);
                PC++;
                FETCHCOUNT_ACTION;
                HISTOGRAM_ACTION(opcode1);
                EXEC;
            } else
            #endif
            #if (PUSH1X2_INSN > 0)
            {
                RECODE(PUSH1X2);  // PUSH1/PUSH1
                next1 = opcode4 >> 8;
                push((WORD_T)next1);
                next1 = opcode4 >> 24;
                push((WORD_T)next1);
                STEPCOUNT_ACTION(1);
                #if ((PUSH1X2_INSN == 2) && defined(RECODE_INSN))
                // TAIL if recoded (high4 not available)
                PC+=3;
                NEXT;
                #else
                // NEXT = FETCH + EXEC
                // TAIL if not recoded (high4 available)
                opcode4=high4;
                PC+=4; // 3 + FETCH
                HISTOGRAM_ACTION(opcode1);
                FETCHCOUNT_ACTION;
                EXEC;
                #endif
            }
            #endif
            { high4=high4<<16 | opcode4>>16;} // allow opcode4 prefetching
        } else
        #endif
        #endif
        {
            #ifdef PATTERN_PUSH1_HIGH4
            high4=*(uint32_t*)(PC+3);
            opcode8 = ((uint64_t)high4 << 32)|opcode4;
            #if (C1TOSTACK8_INSN > 0)
            if (0x17200008060000 == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK8); // PUSH1/GET_SP/PUSH1/ADD/STORE8
                next1_val = opcode4 >> 8;
                next1_addr =*(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint64_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (C1TOSTACK4_INSN > 0)
            if (0x16200008060000 == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK4);    // PUSH1/GET_SP/PUSH1/ADD/STORE4
                next1_val = opcode4 >> 8;
                next1_addr = *(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint32_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (C1TOSTACK2_INSN > 0)
            if (0x15200008060000 == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK2);    // PUSH1/GET_SP/PUSH1/ADD/STORE2
                next1_val = opcode4 >> 8;
                next1_addr = *(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint16_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (C1TOSTACK1_INSN > 0)
            if (0x14200008060000 == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK1);    // PUSH1/GET_SP/PUSH1/ADD/STORE1
                next1_val = opcode4 >> 8; 
                next1_addr = *(PC+3); 
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint8_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (JUMP_PC_1_INSN > 0)
            if (0x0220050000 == (opcode8 & 0x0ffffff0000)){
                RECODE(JUMP_PC_1);    // PUSH1/GET_PC/ADD/JUMP
                next1 = opcode4 >> 8;
                PC += (WORD_T)next1 + 2;
                STEPCOUNT_ACTION(3); NEXT;
            } else
            #endif
            { high4=opcode8 >> 16;} // allow opcode4 prefetching
            #endif
        }
        // PUSH1 followed by any other insn
        RECODE(NEW_PUSH1);
        next1 = opcode4 >> 8;
        push((WORD_T)next1);
        #if (((NEW_PUSH1_INSN == 2) && defined(RECODE_INSN)) || (!defined(PATTERN_PUSH1_HIGH4) && !defined(PATTERN_PUSH1N)))
        // TAIL IF RECODED (opcode8 not available)
        PC+=1;
        NEXT;
        #else
        // TAIL norecode
        // NEXT = FETCH + EXEC
        opcode1 = high4;
        opcode4 = high4;
        PC+=2;
        HISTOGRAM_ACTION(opcode1);
        FETCHCOUNT_ACTION;
        EXEC;
        #endif
    #else
        next1 = *((uint8_t*)PC);
        push((WORD_T)next1);
        PC+=1;
        NEXT;
    #endif

    PUSH2:
    #if OPTENABLED
        #ifdef PATTERN_PUSH2
        high4=*(uint32_t*)(PC+3);
        opcode8 = ((uint64_t)high4 << 32)|opcode4;
        #if (JUMP_PC_2_INSN > 0)
        if (0x022005000000 == (opcode8 & 0x0ffffff000000)){
            RECODE(JUMP_PC_2);    // PUSH2/GET_PC/ADD/JUMP
            next2 = opcode4 >> 8;
            PC += (WORD_T)next2 + 3;
            STEPCOUNT_ACTION(3); NEXT;
        } else
        #endif
        #if (C2TOSTACK8_INSN > 0)
        if (0x1720000806000001 == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK8);    // PUSH2/GET_SP/PUSH1/ADD/STORE8
            next2_val = opcode4 >> 8; 
            next2_addr = *(PC+4); 
            SPplus = (WORD_T)SP + (WORD_T)next2_addr - sizeof(WORD_T);
            PC+=7;
            *((uint64_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (C2TOSTACK4_INSN > 0)
        if (0x1620000806000001 == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK4);    // PUSH2/GET_SP/PUSH1/ADD/STORE4
            next2_val = opcode4 >> 8; 
            next2_addr = *(PC+4); 
            SPplus = (WORD_T)SP + (WORD_T)next2_addr - sizeof(WORD_T);
            PC+=7;
            *((uint32_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (C2TOSTACK2_INSN > 0)
        if (0x1520000806000001 == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK2);    // PUSH2/GET_SP/PUSH1/ADD/STORE2
            next2_val = opcode4 >> 8; 
            next2_addr = *(PC+4); 
            SPplus = (WORD_T)SP + (WORD_T)next2_addr - sizeof(WORD_T);
            PC+=7;
            *((uint16_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (C2TOSTACK1_INSN > 0)
        if (0x1420000806000000 == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK1);    // PUSH2/GET_SP/PUSH1/ADD/STORE1
            next2_val = opcode4 >> 8; 
            next2_addr = *(PC+4); 
            SPplus = (WORD_T)SP + (WORD_T)next2_addr - sizeof(WORD_T);
            PC+=7;
            *((uint8_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #endif
        {
            RECODE(NEW_PUSH2);    // new push2 
            next2 = *((uint16_t*)PC);
            push((WORD_T)next2);
            PC+=2;
            NEXT;
        }
    #else
        next2 = *((uint16_t*)PC);
        push((WORD_T)next2);
        PC+=2;
        NEXT;
    #endif
    PUSH4:
    #if OPTENABLED
        #ifdef PATTERN_PUSH4
        #if (JUMP_PC_4_INSN > 0)
        high4=*(uint32_t*)(PC+3);
        if (0x02200500 == (high4 & 0x0ffffff00)){
            RECODE(JUMP_PC_4);    // PUSH2/GET_PC/ADD/JUMP
            next4 = *(uint32_t*)PC;
            PC += next4 + 5;
            STEPCOUNT_ACTION(3); NEXT;
        } else
        #endif
        #endif
        {
            RECODE(NEW_PUSH4);
            next4 = *((uint32_t*)PC);
            push((WORD_T)next4);
            PC+=4;
            NEXT;
        }
    #else
        next4 = *((uint32_t*)PC);
        push((WORD_T)next4);
        PC+=4;
        NEXT;
    #endif
    //-----------------
    PUSH8:
        next8 = *((uint64_t*)PC);
        push((WORD_T)next8);
        PC+=8;
        NEXT;
    //-----------------
    SIGX1:
        x = pop();
        push((int64_t)((int8_t)(x & (BITMASK8))));
        NEXT;
    SIGX2:
        x = pop();
        push((int64_t)((int16_t)(x & (BITMASK16))));
        NEXT;
    SIGX4:
        x = pop();
        push((int64_t)((int32_t)(x & (BITMASK32))));
        NEXT;
    //-----------------
    LOAD1:
        a = pop();
        push((WORD_T)*((uint8_t*)a));
        NEXT;
    LOAD2:
        a = pop();
        push((WORD_T)*((uint16_t*)a));
        NEXT;
    LOAD4:
        a = pop();
        push((WORD_T)*((uint32_t*)a));
        NEXT;
    LOAD8:
        a = pop();
        push((WORD_T)*((uint64_t*)a));
        NEXT;
    //-----------------
    STORE1:
        u = pop();
        *((uint8_t*)u) = pop(); 
        NEXT;
    STORE2:
        u = pop();
        *((uint16_t*)u) = pop(); 
        NEXT;
    STORE4:
        u = pop();
        *((uint32_t*)u) = pop(); 
        NEXT;
    STORE8:
        u = pop();
        *((uint64_t*)u) = pop(); 
        NEXT;
    //-----------------
    // Arithmetic
    ADD:
        x = pop();
        y = pop();
        push(x+y);
        NEXT;
    MUL:
        x = pop();
        y = pop();
        push(x*y);
        NEXT;
    //-----------------
    // Division by zero handled by signal SIGFPE
    DIV:
        u = pop();
        v = pop();
        push(v / u);
        NEXT;
    REM:
        u = pop();
        v = pop();
        push(v % u);
        NEXT;
    //-----------------
    // Logical works unsigned
    LT:
    #if OPTENABLED
        #ifdef PATTERN_LT
        #if (LT_JZ_INSN > 0)
        if (0x000300 == (opcode4 & 0x00ff00)){
            RECODE(LT_JZ);    // LT/JZ
            u = pop();
            v = pop();
            STEPCOUNT_ACTION(1);
            if (v < u) {
                PC+=2;
                NEXT;
            } else {
                next1s = opcode4 >> 16; 
                PC = (char*)((uint64_t)PC + (int64_t)next1s + 2);
                NEXT;
            }
        } else
        #endif
        #if (LT_NOT_JZ_INSN > 0)
        if (0x00032a00 == (opcode4 & 0x00ffff00)){
            RECODE(LT_NOT_JZ);    // LT/NOT/JZ
            u = pop();
            v = pop();
            STEPCOUNT_ACTION(2);            
            if (v < u) {
                next1s = opcode4 >> 24; 
                PC = (char*)((uint64_t)PC + (int64_t)next1s + 3);
                NEXT;
            } else {
                PC+=3;
                NEXT;
            }
        } else
        #endif
        #endif
        {
            RECODE(NEW_LT);
            u = pop();
            v = pop();
            if (v < u) {
                push(-1);
                NEXT;
            } else {
                push(0);
                NEXT;
            }
        }
    #else
        u = pop();
        v = pop();
        if (v < u) {
            push(-1);
            NEXT;
        } else {
            push(0);
            NEXT;
        }
    #endif

    //-----------------
    AND:
        u = pop();
        v = pop();
        push(u & v);
        NEXT;
    //-----------------
    OR:
        u = pop();
        v = pop();
        push(u | v);
        NEXT;
    //-----------------
    NOT:
        u = pop();
        push(~u);
        NEXT;
    //-----------------
    XOR:
    #if OPTENABLED
        #ifdef PATTERN_XOR
        #if (XOR_1_LT_INSN > 0)
        if (0x24000800 == (opcode4 & 0x0ff00ff00)){
            RECODE(XOR_1_LT);    // XOR/PUSH1/LT
            next1 = opcode4 >> 16; 
            u = pop();
            v = pop();
            uint64_t nu= next1; 
            uint64_t nv= u^v;
            PC+=3;
            STEPCOUNT_ACTION(2);            
            if (nv < nu) {
                push(-1);
                NEXT;
            } else {
                push(0);
                NEXT;
            }
        } else
        #endif
        #endif
        {
            RECODE(NEW_XOR);
            u = pop();
            v = pop();
            push(u ^ v);
            NEXT;
        }
    #else
        u = pop();
        v = pop();
        push(u ^ v);
        NEXT;
    #endif
    //-----------------
    POW2:
        u = pop();
        push(1UL << u);
        NEXT;
    //-----------------
    // OUTPUT
    //-----------------
	#ifdef NO_IO
    PUT_CHAR:
        u = pop();
        putc((int)(unsigned char)u, OUTPUT_PUTCHAR);
        NEXT;
    //-----------------
    PUT_BYTE:
        u = pop();
        putc((int)(unsigned char)u, OUTPUT_OUTPUT);
        NEXT;
    ADD_SAMPLE:
        a = pop(); 
        b = pop(); 
        //NO_IO: printing instruction
        fprintf(OUTPUT_OUTPUT, "add_sample!! %lu %lu\n", a, b);
        NEXT;
    NEW_FRAME:
        a = pop(); 
        b = pop(); 
        u = pop();
        //NO_IO: printing instruction
        fprintf(OUTPUT_OUTPUT, "new_frame!! %lu %lu %lu\n", a, b, u); 
        NEXT;
    SET_PIXEL:
        a = pop(); 
        b = pop(); 
        u = pop();
        v = pop();
        r = pop();
        //NO_IO: printing instruction
        fprintf(OUTPUT_OUTPUT, "set_pixel!!!!! %lu %lu %lu %lu %lu\n", a, b, u, v, r); 
        NEXT;
    READ_PIXEL:
        a = pop(); 
        b = pop(); 
        //NO_IO: printing instruction
        fprintf(OUTPUT_OUTPUT, "read_pixel!! %lu %lu\n", a, b); 
        push(0);
        NEXT;
    READ_FRAME:
        a = pop();
        fprintf(OUTPUT_OUTPUT, "read_frame! %lu\n", a);
        push(0);
        push(0);
        NEXT;
	#else
    // from github.com/preservationvm/ivm-implementations/blob/master/OtherMachines/vm.c
	READ_FRAME:
		ioReadFrame(pop(), &u, &v); // u -> x ; v -> y
		push(u);
		push(v);
        NEXT;
    READ_PIXEL:
		v = pop(); u = pop();
		push(ioReadPixel(u, v)); // u -> x ; v -> y
        NEXT;
    NEW_FRAME:
		r = pop(); v = pop(); u = pop();
		ioFlush(); ioNewFrame(u, v, r);
		printf("\r\f");
        NEXT;
    SET_PIXEL:
		b = pop(); a = pop(); r = pop();
		v = pop(); u = pop();
		ioSetPixel(u, v, r, a, b); // u -> x ; v -> y ; a -> r
        NEXT;
    ADD_SAMPLE: u = pop(); v = pop(); ioAddSample(v, u); NEXT; // u -> x ; v -> y

    PUT_CHAR: ioPutChar(pop()); NEXT;
    PUT_BYTE: ioPutByte(pop()); NEXT;
	#endif
	

    HALT:

    signal(SIGINT,SIG_DFL);
    uint64_t retval = *((uint64_t*)SP);
    
    #ifdef WITH_IO
    ioFlush();
        #ifdef PARALLEL_OUTPUT
        waitUntilProcessed(queueHandler);
        #endif
    #endif
    fprintf(OUTPUT_MSG, "\n");

    #ifdef STEPCOUNT
        #if (defined(HISTOGRAM) && !defined(NOOPT))
        fprintf(OUTPUT_MSG, "Executed %lu instructions; %lu fetches (%4.2lf insn per fetch)\n\n",
                            steps, fetchs, (double)steps/fetchs);   
        #else
        fprintf(OUTPUT_MSG, "Executed %lu instructions\n\n", steps);
        #endif
    #endif

    #ifdef HISTOGRAM
    for (int i=0; i < 256; i++) {
        if (histogram[i] > 0) {
            printf("%10ld\t%-10s\t%6.3lf%%\t%10ld\t%15.2lf\n",
                    histogram[i], insn_attributes[i].name,
                    (double)histogram[i]/fetchs*100, histo2[i],
                    histo2[i]?(double)histogram[i]/histo2[i]:histogram[i]);
        }
    }
    #endif

    // Print the stack in the same format than the vm implementation
    print_stack(FORMAT_STACK_IVM, 1<<31);

    if (error){
        if (error == SIGFPE) {
            printf("error: division by zero\n");
        } else if (error == SIGSEGV) {
            printf("error: segmentation fault\n");
        } else if (error == SIGINT) {
            printf("Program terminated by user request ^C\n");
        }
        exit(EXIT_FAILURE);
    }

    return (int)retval;
}
