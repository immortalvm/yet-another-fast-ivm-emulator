/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Original emulator:
 Authors:
  Eladio Gutierrez Carrasco
  Sergio Romero Montiel
  Oscar Plata Gonzalez

 Date: Mar 2020 - Feb 2021 - Jun 2023 - May 2024


 Input/output version:
 IO stuff in file ivm_io.c, fucntion extracted from Ivar Rummelhoff code in:
 github.com/preservationvm/ivm-implementations/blob/master/OtherMachines/vm.c
 Requeriments:
 * install libpng: sudo apt-get install libpng-dev
 * compile ivm_emu with options: -DWITH_IO -lpng and link with ivm_io

 Date: May 2020


 Parallel version:
     Based on creating one independent process for writing each frame
 * compile ivm_emu with options: -DWITH_IO -lpng -DPARALLE_OUTPUT


 Some ideas from http://www.w3group.de/stable.html

*/

/*
 Compile with:
    make # generates the next three executables
    gcc -Ofast ivm_emu.c   # The fastest one without input/ouput
	gcc -Ofast -DWITH_IO   ivm_emu.c ivm_io.c -lpng  # Enable IO instruction
    gcc -Ofast -DWITH_IO   ivm_emu.c ivm_io.c -lpng -DPARALLEL_OUTPUT # Enable parallel output

    gcc -Ofast -DVERBOSE=1 ivm_emu.c  # Enable verbose
    gcc -Ofast -DVERBOSE=2 ivm_emu.c  #
    gcc -Ofast -DVERBOSE=3 ivm_emu.c  # Enable verbose (trace), compact format
    gcc -Ofast -DVERBOSE=4 ivm_emu.c  # Enable verbose (trace), detailed format

    gcc -Ofast -DSTEPCOUNT ivm_emu.c  # Enable instruction count
    gcc -Ofast -DNOOPT     ivm_emu.c  # Disable optimizations
    gcc -Ofast -DHISTOGRAM ivm_emu.c  # Enable insn. pattern histogram

 Number of processes for the parallel version:
 * Default: 8
 * if compiled with -DNUM_THREADS=N1, N1 is used instead of the default value
 * if environment variable export NUM_THREADS=N2, N2 is used instead of N1 or default
 * In any case, the parallel version uses at least 2 processes, in general:
            1 for emulation and (N-1) processes for io
 Note that this number refers to forked processes, although named NUM_THREADS
*/

// Version v2.1.5 compatible with ivm implementation v2.1
#ifdef WITH_IO
    #ifdef PARALLEL_OUTPUT
    #define VERSION  "v2.1.5-fast-io-parallel"
    #else
    #define VERSION  "v2.1.5-fast-io"
    #endif
#else
    #define VERSION  "v2.1.5-fast"
#endif

#ifndef IVM_TERMIOS
#define IVM_TERMIOS 1
#endif

////////////////////////////////////////////////////////////////////////////////
// Define FPE_ENABLED to enable FPE exception for "divide by zero" instead
// of returning 0 when dividing by zero
#define FPE_ENABLED_ 

////////////////////////////////////////////////////////////////////////////////
// Disable optimizations: -DNOOPT
#if defined(NOOPT)
#define OPTENABLED  0
#else
#define OPTENABLED  1
#endif

////////////////////////////////////////////////////////////////////////////////
// The FOLLOWING is only available if OPTENABLED==1
// macros definition before include emulator header file 'ivm_emu.h'
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
    #define PATTERN_GETSP_STORE
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
#ifdef PATTERN_GETSP_STORE
    #define FAST_POP_INSN    2
    #define FAST_POP2_INSN   2
#endif
#ifdef PATTERN_PUSH0
    #define SHORT_JUMPF_INSN 2
    #define SHORT_JUMPB_INSN 2
    #define XOR_0_INSN       2
    #define NOT_0_MUL_INSN   2
    #define PUSH0X2_INSN     2
    #define PUSH0X3_INSN     2
    #define PUSH0X4_INSN     2
#endif
#ifdef PATTERN_PUSH1_ALU
    #define LT_1_JZF_INSN    2
    #define LT_1_JZB_INSN    2
    #define NOT_1_ADD_INSN   2
    #define LT_1_NOT_INSN    2
    #define LT_1_JNZF_INSN   2
    #define LT_1_JNZB_INSN   2
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
    #define C2TOSTACK1_INSN  2
    #define C2TOSTACK2_INSN  2
    #define C2TOSTACK4_INSN  2
    #define C2TOSTACK8_INSN  2
#endif
#ifdef PATTERN_PUSH4
    #define JUMP_PC_4_INSN   2
#endif
#ifdef PATTERN_LT
    #define LT_JZF_INSN      2
    #define LT_NOT_JZF_INSN  2
    #define LT_JZB_INSN      2
    #define LT_NOT_JZB_INSN  2
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
#include <locale.h>
#include <termios.h>
// include emulator header file after defines
#include "ivm_emu.h"

// Some error codes (raise with longjmp like a signal does)
#define WRONG_BINARY_VERSION 1009
#define WRONG_BINARY_VERSION_RET_VALUE 9

// Global options
// Default memory size in bytes
#define MEMBYTES (512UL*1024*1024)
char *opt_bycodefile = NULL;           // Binary bytecode file name
unsigned long opt_maxmem = MEMBYTES;   // Memory size (-m <number>)
char* argFile = NULL;
char* envFile = NULL;
char* inpDir = NULL;
char* outDir = NULL;


#if defined(WITH_IO)
#undef NO_IO
// IO instructions
#include "ivm_io.h"
#else
#define NO_IO
#undef PARALLEL_OUTPUT
#endif

// Output stream for the simulation 
#define OUTPUT_PUTCHAR stderr
#define OUTPUT_PUTBYTE stderr

// Output stream for error messages and putchar, respectively
// By default: stdout for program messages and stderr for put_char
#define OUTPUT_MSG stdout

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

// Global stuff
char *PC;               // Program counter
char *SP;               // Stack pointer
char *Mem;              // The memory
unsigned long MemBytes; // Size of the memory
void* addr[256];        // Where to go to execute the instruction


long segment_start = 0; // Where to load the bytecode
                        // Mem[segment_start] is the
                        // first byte of the program

//Program bytes are placed from Mem[execStart] to Mem[execEnd],
//both included
unsigned long execStart = 0; // First position of the bytecode in memory
unsigned long execEnd= 0;    // Last position of the bytecode in memory
unsigned long argStart = 0;  // First position of the argument file in memory
unsigned long argEnd= 0;     // Last position of the argument file in memory
unsigned long envStart = 0;  // First position of the environment file in memory
unsigned long envEnd= 0;     // Last position of the environment file in memory

typedef struct insn_attr {
    const char *name; // Name of the instruction
    int opbytes;      // Number of bytes of the immediate operand
                      // As the opcode is 1-byte long, the total size
                      // of the instruction is:
                      // opbytes + 1
} insn_attr_t;
// Array of attributes for all the instruction set
insn_attr_t insn_attributes[256];

#ifdef HISTOGRAM
unsigned long histogram[256];
unsigned long histo2[256];
#endif

#if (VERBOSE > 0)
unsigned long samples[256];
#endif

#ifdef PARALLEL_OUTPUT
#if !defined(NUM_THREADS)
    // Default number of processes
    int maxproc = 4;
#else
    // defined with -DNUM_THREADS=N at compile time
    int maxproc = NUM_THREADS;
#endif

int child = 0;
unsigned int nproc=0;
int status;
#endif


/*
    Process command line options
*/
int get_options(int argc, char* argv[]) {
    int i, c;
    int na = 0; // Number of appearances of flag '-a': ivm_emu ... -a first -a second ...
    while ((c = getopt(argc, argv, "m:o:i:a:L:")) != -1) {
        switch (c) {
          case 'm': opt_maxmem = atol(optarg)>0?atol(optarg):opt_maxmem; break;
          case 'o': outDir = optarg; break;
          case 'i': inpDir = optarg; break;
          case 'a': if (na==0) {argFile = optarg; na++; break;} // The 1st. -a argument is the argument file
                    if (na==1) {envFile = optarg; na++; break;} // The 2nd. -a argument is the environment file
                    break;
          case 'L': segment_start = atol(optarg)>0?atol(optarg):0; break;
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
                            "[-a <arg file> [-a <env file>]] <ivm binary file>\n",
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
        if (envFile)
            fprintf(OUTPUT_MSG, "envFile=%s\n", envFile);
    #endif

    return 1;
}

// Address translation
char* idx2addr(unsigned long i);
unsigned long addr2idx(char* p);
inline char* idx2addr(unsigned long i){ return (char *)&(Mem[i]); }
inline unsigned long addr2idx(char* p){ return (unsigned long)p - (unsigned long)Mem; }

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

//#if (VERBOSE >= 3)
#include "ivm_emu_hash_table.h"
sym_table_t *Ts = NULL; // global struct for the symbol table

/*
  Get the name of symbol file from binary filename.
  Basically, replace .b or .bin extension by .sym
*/
char *get_ivm_sym_filename(char *binfilename){
    // The sym filename is got by replacing binary file extension by '.sym' in the binary filename
    char *sym_ext = ".sym";
    long l = strlen(binfilename);
    char *fn = (char*)calloc(l*sizeof(char) + sizeof(sym_ext) + 8, 1);
    strncpy(fn, binfilename, l+1);
    // Replace binary file extension by '.sym'
    char *p = fn + l;
    do{ p--; } while (p>=fn && *p != '.');
    if (p<fn) p = fn + l;
    strcpy(p, sym_ext);
    return fn;
}

/*
    Read a ivm sym file with the symbol table.
    Return the number of labels found.
    (this may be optional, so this function never calls exit())

    The sym file has the following contents, with pairs (label, pc)
    after the section '--Labels--':
        --Previous--

        --Size--
        3609
        --Relative--
        --Constant--
        --Labels--
        z/main	686
        z/.LIVM_next	3424
        z/.credits	3419
        z/_start	50
        z/_exit	700
        z/.L7.l.55d._call_atexit.c	1093
        z/.XSC	3461
        z/__IVM64_exit_jb__	798
            ...
        --Spacers--
*/
long ivm_read_sym(char *filename)
{
    FILE *fd;
    fd = fopen(filename, "r");
    if (!fd){
        //fprintf(OUTPUT_MSG, "Can't open sym file '%s'\n", filename);
        return 0;
    }

    long nlabels=0; // Number of labels read
    #define SYM_FILE_MAXLINE 4096
    char line[SYM_FILE_MAXLINE], label[SYM_FILE_MAXLINE], *l;

    do {
        l = fgets(line, SYM_FILE_MAXLINE-1, fd); // note that gets includes \n
    } while (l && strcmp("--Labels--\n", line));

    if (strcmp("--Labels--\n", line)){
        // The open file has no --Labels-- section
        return 0;
    }

    int updatesym = 0; // If a same PC is associated to several labels,
                       // do not update it in the symbol table, thus keep the first one
                       // in the .sym file

    // Avoid stack smashing in fscanf (The Practice of Programming, Kernighan and Pike)
    char format[32];
    snprintf(format, sizeof(format), "%%%ds %%ld", (int)(sizeof(label)-1));
    do {
        long pclabel;
        //long nreads = fscanf(fd, "%s %ld", label, &pclabel);
        long nreads = fscanf(fd, format, label, &pclabel);
        if (nreads != 2) break;
        putsym_hash(Ts, pclabel, label, updatesym);
        nlabels++;
    } while (1);

    fclose(fd);
    return nlabels;
}
//#endif // VERBOSE >= 3


/*
    Print the last n elements of the stack
    FORMAT_STACK_ROW: for tracing the stack, elements in a row
    FORMAT_STACK_IVM_COMPACT_ROW: for tracing the stack, elements
                      in a row like the ivm application
    FORMAT_STACK_IVM: to print at the end the resulting stack
                      like the ivm application
*/

enum format_stack {FORMAT_STACK_ROW, FORMAT_STACK_IVM, FORMAT_STACK_IVM_COMPACT_ROW};
void print_stack(int format, unsigned long n){
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

    // Format of ivm implementation for the final stack after the program ends
    if (format == FORMAT_STACK_IVM) {
        fprintf(OUTPUT_MSG, "End stack:\n");
        for (p=SP, i=1; p<=p_start; p=(char*)(((WORD_T*)p)+1), i++){
            WORD_T val = *((WORD_T*)p);
            fprintf(OUTPUT_MSG, "0x..%06lx %8ld\n", val & 0xffffff, val);
        }
        fprintf(OUTPUT_MSG, "\n");
    }

    // Format of the ivm implementation for each line of the trace
    if (format == FORMAT_STACK_IVM_COMPACT_ROW) {
        fprintf(OUTPUT_MSG, " start+%#lx: ", addr2idx(SP));
        for (p=p_start; p>=SP; p=(char*)(((WORD_T*)p)-1)){
            WORD_T val = *((WORD_T*)p);
            //fprintf(OUTPUT_MSG, " %#lx", val);

            // Values in the range of memory will be printed as
            // "@start+offset"
            char* valp = (char*)val;
            if (valp <= p_start && valp>=Mem) {
                fprintf(OUTPUT_MSG, " @start+%#-6lx", addr2idx(valp));
            } else {
                fprintf(OUTPUT_MSG, " %ld", val);
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

    fprintf(OUTPUT_MSG, "%#016lx-..\t", start);
    for (i = start; i <= end; i++){
        for (k = i; k <= MIN(i + rowbytes - 1, end); k++) {
            fprintf(OUTPUT_MSG, "%02x  ", (unsigned char)Mem[k]);
        }
        i = k-1;
        fprintf(OUTPUT_MSG, "\n");
        fprintf(OUTPUT_MSG, "%#016lx-..\t", k);
    }
    fprintf(OUTPUT_MSG, "\n\n");
}
#endif

#if (VERBOSE >= 2)
/*
    Print the value of SP and the last n elements of the stack
*/
void print_stack_status(){
    fprintf(OUTPUT_MSG, "\tSP = %p\n", SP);
    fprintf(OUTPUT_MSG, "\tTOS = %#lx\n", (*(WORD_T*)SP) );
    print_stack(FORMAT_STACK_ROW, 16);
}

/*
    Print the value of SP and the last n elements of the stack
    Compact version (similar to F# ivm implementation)
*/
void print_stack_status_compact(char *pc){
    static int first = 1; // In the first line of the trace,
                          // do no print the stack (left blank)
    if (first) {
        first = 0;
    } else {
        print_stack(FORMAT_STACK_IVM_COMPACT_ROW, 16);
    #if (VERBOSE >= 3)
        // Print a line with the label corresponding to this PC
        // if it was inserted in the symbol table
        // This line with the label has this format:
        // -- z/.LC012 --
        symrec* r = getsym_hash(Ts, addr2idx(pc));
        if (r) {
            fprintf(OUTPUT_MSG, "-- %s --\n", r->label);
        }
     #endif
    }
    // This PC,INSN information is printed for the next trace line
    unsigned char op_code = *pc;
    fprintf(OUTPUT_MSG, "start+%#-9lx: %-9s ",
                        addr2idx(pc), insn_attributes[op_code].name);
}
#endif

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
    #if (VERBOSE >= 3)
        // Print the label corresponding to this PC value
        symrec* r = getsym_hash(Ts, addr2idx(pc));
        if (r) {
            fprintf(OUTPUT_MSG, "--> %s (=%ld)\n", r->label, r->pc);
        }
     #endif
}

void reset_std_streams()
{
    // Restore stream orientation after just in case wprintf()
    // and printf() were mixed up
    // Note that streams where IVM writes need to be not buffered
    if (freopen(NULL, "a", OUTPUT_PUTCHAR)){};
    if (freopen(NULL, "a", OUTPUT_PUTBYTE)){};
    setvbuf(OUTPUT_PUTCHAR, NULL, _IONBF, 0);
    setvbuf(OUTPUT_PUTBYTE, NULL, _IONBF, 0);
}

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


int main(int argc, char* argv[])
{
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

    // Trace ON/OFF through given opcodes
    // Compile with VERBOSE to see the effect
    #if (VERBOSE > 2)
    uint8_t trace = 1;
    #elif (VERBOSE == 2)
    uint8_t trace = 0;
    #endif
    uint8_t probe = 0;
    uint8_t read_probe;

    char *filename;

    #if (!IVM_TERMIOS)
    #define TTY_DEF
    #define TTY_NEW
    #else
    struct termios tty_def, tty_new;
    tcgetattr(STDIN_FILENO, &tty_def);  // save terminal characteristics
    tty_new = tty_def;
    tty_new.c_lflag &= ~(ICANON | ECHO);
    tty_new.c_cc[VTIME] = 0;
    tty_new.c_cc[VMIN] = 1;
    #define TTY_DEF do{tcsetattr(STDIN_FILENO, TCSANOW, &tty_def );}while(0)
    #define TTY_NEW do{tcsetattr(STDIN_FILENO, TCSANOW, &tty_new );}while(0)
    #endif

    fprintf(OUTPUT_MSG, "Yet another ivm emulator, %s\n", VERSION);
    fprintf(OUTPUT_MSG, "Compatible with ivm-2.1\n");
    char *str = "Compiled with:";
    #if (VERBOSE>0)
        fprintf(OUTPUT_MSG, "%s -DVERBOSE=%d", str, VERBOSE);
        str = "";
    #endif
    #ifdef NOOPT
        fprintf(OUTPUT_MSG, "%s -DNOOPT", str);
        str = "";
    #endif
    #ifdef STEPCOUNT
        fprintf(OUTPUT_MSG, "%s -DSTEPCOUNT", str);
        str = "";
    #endif
    #ifdef HISTOGRAM
        fprintf(OUTPUT_MSG, "%s -DHISTOGRAM", str);
        str = "";
    #endif
    #if defined(WITH_IO)
        fprintf(OUTPUT_MSG, "%s -DWITH_IO", str);
        str = "";
    #endif
    #ifdef PARALLEL_OUTPUT
        fprintf(OUTPUT_MSG, "%s -DPARALLEL_OUTPUT", str);
        str = "";
    #endif
    #ifdef FPE_ENABLED 
        fprintf(OUTPUT_MSG, "%s -DFPE_ENABLED", str);
        str = "";
    #endif
    if (str[0] == '\0') printf("\n");

    if (!get_options(argc, argv)){
        exit(EXIT_FAILURE);
    }

    // get/set locale
    #define ALT_LOCALE "C.UTF-8"
    char *locale;
    if (((locale = setlocale(LC_CTYPE, "")) == NULL) ||
        ((strstr(locale, "UTF-8") == NULL) && (strstr(locale, "UTF8") == NULL) &&
         (strstr(locale, "utf-8") == NULL) && (strstr(locale, "utf8") == NULL))) {
        fprintf(OUTPUT_MSG, "locale = %s\nTrying to change to \"%s\"...", locale, ALT_LOCALE);
        if (setlocale(LC_CTYPE, ALT_LOCALE) == NULL) {
            fprintf(OUTPUT_MSG, "couldn't change locale\n");
            fprintf(OUTPUT_MSG, "Multibyte characters may not work properly.\n");
        } else {
            fprintf(OUTPUT_MSG, "OK\n");
        }
    }

    // Get options
    filename = opt_bycodefile;
    MemBytes = opt_maxmem;

    // Prepare the memory
    Mem = (char*)malloc(MemBytes * sizeof(char));
    memset(Mem, 0, MemBytes);

    
    #ifndef NO_IO
    ioInitIn();
    ioInitOut();
    #ifdef PARALLEL_OUTPUT
        // if environment variable NUM_THREADS=N exists, this value is used instead
        // of any previous value as the maximum number of processes
        char *nthreads_str = getenv("NUM_THREADS");
        if (nthreads_str) maxproc = atoi(nthreads_str);
        // Must be 2 or more: 1 thread for emulation and (N-1) threads for io
        maxproc = (maxproc<1)?1:maxproc;
        #if (VERBOSE > 0)
        fprintf(OUTPUT_MSG, "maxproc=%d\n", maxproc);
        #endif
    #endif
    #endif

    fprintf(OUTPUT_MSG,"\n");

    // Instruction Set.
    init_insn_addr(addr);
    init_insn_attributes(insn_attributes);

    // Read bytecode file
    ivm_read_bin(filename, segment_start, &execStart, &execEnd);
    #if (VERBOSE>0)
    fprintf(OUTPUT_MSG, "First byte of the program indexed by %#lx (=%ld), last byte by %#lx (=%ld)\n",
            execStart, execStart, execEnd, execEnd);
    fprintf(OUTPUT_MSG, "\n");
    #endif

    // Read sym file if available (to show labels when tracing or in case of error)
    //#if (VERBOSE >= 3)
    char *symfile = get_ivm_sym_filename(filename);

    Ts = init_symtable(12346791);
    long nsym = ivm_read_sym(symfile);  // ivm_read_sym uses the global symbol table Ts

    #if (VERBOSE >=1)
    if (nsym>0) {
        fprintf(OUTPUT_MSG, "Read %ld symbols from sym file '%s'\n\n", nsym, symfile);
    } else {
        fprintf(OUTPUT_MSG, "No labels from .sym file\n"
                            "(perhaps no .sym file found; .sym file must have "
                            "the same name as binary, replacing extension by .sym)\n\n");
    }
    //print_symtable(Ts); //debug
    #endif
    free(symfile);
    //#endif

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

    // Read environment file (it follows argument file)
    if (envFile){
        ivm_read_bin(envFile, argEnd+9, &envStart, &envEnd);
        *(uint64_t*)(&Mem[argEnd+1]) = envEnd - envStart + 1;
        #if (VERBOSE>0)
        fprintf(OUTPUT_MSG, "First byte of the environment file indexed by %#lx (=%ld), last byte by %#lx (=%ld)\n",
                envStart, envStart, envEnd, envEnd);
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
        #define VERBOSE_ACTION do{ if (trace>1) print_stack_status(); if (trace) print_insn(PC-1);} while(0)
    #elif (VERBOSE == 3)
        #define VERBOSE_ACTION do{ print_stack_status_compact(PC-1);} while(0)
    #elif (VERBOSE >= 4)
        #define VERBOSE_ACTION do{ print_stack_status(); print_insn(PC-1);} while(0)
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
        unsigned long fetchs = 0;   // Fetch count
        unsigned long samples[256]; // Instruction count
        bzero(samples, 256*sizeof(unsigned long));
        #define STEPCOUNT_ACTION(n)  do{samples[probe]+=n;}while(0)
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

    reset_std_streams();
    TTY_DEF;

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
        #define PATTERN_NOP2_CODE (OPCODE_NOP<<8 | OPCODE_NOP)
        #define PATTERN_NOP4_CODE (PATTERN_NOP2_CODE<<16 | PATTERN_NOP2_CODE)
            #if (NOP4_INSN > 0 || NOP8_INSN > 0)
            if (opcode4 == PATTERN_NOP4_CODE) {
                #if (NOP8_INSN > 0)
                if (*(uint32_t*)(PC+3) == PATTERN_NOP4_CODE) {
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
            if (opcode4 == PATTERN_NOP2_CODE ) {
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
    JZ_FWD:
        next1 = *((uint8_t*)PC);
        PC+=1;
        a = pop();
        if (a == 0){
            PC += next1;
        }
        NEXT;
    //-----------------
    JZ_BACK:
        next1 = *((uint8_t*)PC);
        PC+=1;
        a = pop();
        if (a == 0){
            PC -= next1 + 1;
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
        #define PATTERN_GETPC_PUSH1_ADD_CODE (OPCODE_ADD<<24 | OPCODE_PUSH1<<8)
        #define PATTERN_GETPC_PUSH1_ADD_MASK (0xff<<24 | 0xff<<8)
        if (PATTERN_GETPC_PUSH1_ADD_CODE == (opcode4 & PATTERN_GETPC_PUSH1_ADD_MASK)){
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
        #define PATTERN_GETPC_PUSH2_ADD_CODE (OPCODE_PUSH2<<8)
        #define PATTERN_GETPC_PUSH2_ADD_MASK (0xff<<8)
        if (PATTERN_GETPC_PUSH2_ADD_CODE == (opcode4 & PATTERN_GETPC_PUSH2_ADD_MASK)){
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
        #define PATTERN_GETPC_PUSH4_ADD_CODE (OPCODE_PUSH4<<8)
        #define PATTERN_GETPC_PUSH4_ADD_MASK (0xff<<8)
        if (PATTERN_GETPC_PUSH4_ADD_CODE==(opcode4 & PATTERN_GETPC_PUSH4_ADD_MASK)) {
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
        #define PATTERN_GETPC_PUSH8_ADD_CODE (OPCODE_PUSH8<<8)
        #define PATTERN_GETPC_PUSH8_ADD_MASK (0xff<<8)
        if (PATTERN_GETPC_PUSH8_ADD_CODE==(opcode4 & PATTERN_GETPC_PUSH8_ADD_MASK)) {
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
        #define PATTERN_GETSP_PUSH1_ADD_CODE (OPCODE_ADD<<24 | OPCODE_PUSH1<<8)
        #define PATTERN_GETSP_PUSH1_ADD_MASK (0xff<<24 | 0xff<<8)
        if (PATTERN_GETSP_PUSH1_ADD_CODE == (opcode4 & PATTERN_GETSP_PUSH1_ADD_MASK)) {
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
        if (((OPCODE_NOT<<24 | OPCODE_PUSH1<<8) == (opcode4 & 0x0ff00ff00)) &&
            (*(uint16_t*)(PC+3)==(OPCODE_SET_SP<<8|OPCODE_ADD))) {
            RECODE(DEC_SP_1);
            next1 = opcode4 >> 16;
            u = next1;
            SP = SP + ~u;
            PC+=5; STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (SP_1_INSN > 0)
        if (OPCODE_PUSH1<<8 == (opcode4 & 0x0ff00)) { // not followed by add
            RECODE(SP_1);    // get_sp/push1
            push((WORD_T)SP);
            next1 = opcode4 >> 16;
            push(next1);
            PC+=2; STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #endif
        #ifdef PATTERN_GETSP_PUSH2_ADD
        if (OPCODE_PUSH2<<8 == (opcode4 & 0x0ff00)) {
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
        #ifdef PATTERN_GETSP_STORE
        if (OPCODE_STORE1<<8 == (opcode4 & 0x0fc00)) { // 20, 21, 22, 23
            #if (FAST_POP2_INSN > 0)
            if ((OPCODE_STORE1<<24 | OPCODE_GET_SP<<16) == (opcode4 & 0x0fcff0000)) {
                RECODE(FAST_POP2);
                SP+=16;
                PC+=3; STEPCOUNT_ACTION(3); NEXT;
            } else
            #endif
            #if (FAST_POP_INSN > 0)
            {
                RECODE(FAST_POP);
                SP+=8;
                PC+=1; STEPCOUNT_ACTION(1); NEXT;
            }
            #else
            {
                push((WORD_T)SP);
                NEXT;
            }
            #endif
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
        #if (SHORT_JUMPF_INSN > 0)
        if (OPCODE_JZ_FWD<<8 == (opcode4 & 0x0ff00)){
            RECODE(SHORT_JUMPF);    // PUSH0/JZ_FWD
            next1 = opcode4 >> 16;
            PC += next1 + 2;
            STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #if (SHORT_JUMPB_INSN > 0)
        if (OPCODE_JZ_BACK<<8 == (opcode4 & 0x0ff00)){
            RECODE(SHORT_JUMPB);    // PUSH0/JZ_FWD
            next1 = opcode4 >> 16;
            PC -= next1 - 1;
            STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #if (XOR_0_INSN > 0)
        if (OPCODE_XOR<<8 == (opcode4 & 0x00ff00)) { // PUSH0/XOR = NOP
            RECODE(XOR_0);    // push0/xor
            PC++; STEPCOUNT_ACTION(1); NEXT;
        } else
        #endif
        #if (NOT_0_MUL_INSN > 0)
        if ((OPCODE_MUL<<16 | OPCODE_NOT<<8) == (opcode4 & 0x0ffff00)) { // *(-1)
            RECODE(NOT_0_MUL);    // push0/not/mul
            u=pop();
            push(~u+1);
            PC+=2; STEPCOUNT_ACTION(2); NEXT;
        } else
        #endif
        #if (PUSH0X4_INSN > 0)
        if ((OPCODE_PUSH0<<24 | OPCODE_PUSH0<<16 | OPCODE_PUSH0<<8) == (opcode4 & 0x0ffffff00)) {
            RECODE(PUSH0X4);    // push0/push0/push0/push0
            SP-=BYTESPERWORD*4;
            *((WORD_T*)SP)=(WORD_T)0;
            *((WORD_T*)SP+1)=(WORD_T)0;
            *((WORD_T*)SP+2)=(WORD_T)0;
            *((WORD_T*)SP+3)=(WORD_T)0;
            PC+=3; STEPCOUNT_ACTION(3); NEXT;
        } else
        #endif
        #if (PUSH0X3_INSN > 0)
        if ((OPCODE_PUSH0<<16 | OPCODE_PUSH0<<8) == (opcode4 & 0x0ffff00)) {
            RECODE(PUSH0X3);    // push0/push0/push0
            SP-=BYTESPERWORD*3;
            *((WORD_T*)SP)=(WORD_T)0;
            *((WORD_T*)SP+1)=(WORD_T)0;
            *((WORD_T*)SP+2)=(WORD_T)0;
            PC+=2; STEPCOUNT_ACTION(2); NEXT;
        } else
        #endif
        #if (PUSH0X2_INSN > 0)
        if (OPCODE_PUSH0<<8 == (opcode4 & 0x0ff00)) {
            RECODE(PUSH0X2);    // push0/push0
            SP-=BYTESPERWORD*2;
            *((WORD_T*)SP)=(WORD_T)0;
            *((WORD_T*)SP+1)=(WORD_T)0;
            PC++; STEPCOUNT_ACTION(1); NEXT;
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
        if (OPCODE_POW2<<16 == (opcode4 & 0x0ff0000)){
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
        #if (LT_1_JZF_INSN > 0)
        if ((OPCODE_JZ_FWD<<24 | OPCODE_LT<<16) == (opcode4 & 0x0ffff0000)){
            RECODE(LT_1_JZF);    // PUSH1/LT/JZ_FWD
            next1 = opcode4 >> 8;
            u = next1;
            v = pop();
            STEPCOUNT_ACTION(2);
            if (v < u) {
                PC+=4; NEXT;
            } else {
                next1 =*(PC+3);
                PC += next1 + 4;
                NEXT;
            }
        } else
        #endif
        #if (LT_1_JZB_INSN > 0)
        if ((OPCODE_JZ_BACK<<24 | OPCODE_LT<<16) == (opcode4 & 0x0ffff0000)){
            RECODE(LT_1_JZB);    // PUSH1/LT/JZ_BACK
            next1 = opcode4 >> 8;
            u = next1;
            v = pop();
            STEPCOUNT_ACTION(2);
            if (v < u) {
                PC+=4; NEXT;
            } else {
                next1 =*(PC+3);
                PC -= next1 - 3;
                NEXT;
            }
        } else
        #endif
        #if ((LT_1_NOT_INSN > 0) || (LT_1_JNZF_INSN > 0))
        if ((OPCODE_NOT<<24 | OPCODE_LT<<16) == (opcode4 & 0x0ffff0000)){
            #if (LT_1_JNZF_INSN > 0)
            if (*(uint8_t*)(PC+3) == OPCODE_JZ_FWD) {
                RECODE(LT_1_JNZF);    // PUSH1/LT/NOT/JZ_FWD
                next1 = opcode4 >> 8;
                u = next1;
                v = pop();
                STEPCOUNT_ACTION(2);
                if (v >= u) {
                    PC+=5; NEXT;
                } else {
                    next1 =*(PC+4);
                    PC = (char*)((uint64_t)PC + (int64_t)next1 + 5);
                    NEXT;
                }
            } else
            #endif
            #if (LT_1_JNZB_INSN > 0)
            if (*(uint8_t*)(PC+3) == OPCODE_JZ_BACK) {
                RECODE(LT_1_JNZB);    // PUSH1/LT/NOT/JZ_FWD
                next1 = opcode4 >> 8;
                u = next1;
                v = pop();
                STEPCOUNT_ACTION(2);
                if (v >= u) {
                    PC+=5; NEXT;
                } else {
                    next1 =*(PC+4);
                    PC -= next1 - 4;
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
        if ((OPCODE_ADD<<24 | OPCODE_NOT<<16) == (opcode4 & 0x0ffff0000)){
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
        if (OPCODE_PUSH1<<16 == (opcode4 & 0x0ff<<16)) {
            #if (PUSH1X4_INSN > 0)
            high4=*(uint32_t*)(PC+3);
            if ((OPCODE_PUSH1<<16 | OPCODE_PUSH1) == (high4 & 0x000ff00ff)) {
                RECODE(PUSH1X4); // PUSH1/PUSH1/PUSH1/PUSH1
                #if (PUSH1X4_INSN == 2)
                high4=*(uint32_t*)(PC+3);
                #endif
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
                    (opcode4 & 0x000ff00ff)==(OPCODE_PUSH1<<16 | OPCODE_PUSH1);
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
                opcode1=high4;
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
            if ((1UL*OPCODE_STORE8<<48 | 1UL*OPCODE_ADD<<40 | OPCODE_PUSH1<<24 | OPCODE_GET_SP<<16)
                == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK8); // PUSH1/GET_SP/PUSH1/ADD/STORE8
                next1_val = opcode4 >> 8;
                next1_addr =*(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint64_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (C1TOSTACK4_INSN > 0)
            if ((1UL*OPCODE_STORE4<<48 | 1UL*OPCODE_ADD<<40 | OPCODE_PUSH1<<24 | OPCODE_GET_SP<<16)
                == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK4);    // PUSH1/GET_SP/PUSH1/ADD/STORE4
                next1_val = opcode4 >> 8;
                next1_addr = *(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint32_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (C1TOSTACK2_INSN > 0)
            if ((1UL*OPCODE_STORE2<<48 | 1UL*OPCODE_ADD<<40 | OPCODE_PUSH1<<24 | OPCODE_GET_SP<<16)
                == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK2);    // PUSH1/GET_SP/PUSH1/ADD/STORE2
                next1_val = opcode4 >> 8;
                next1_addr = *(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint16_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (C1TOSTACK1_INSN > 0)
            if ((1UL*OPCODE_STORE1<<48 | 1UL*OPCODE_ADD<<40 | OPCODE_PUSH1<<24 | OPCODE_GET_SP<<16)
                == (opcode8 & 0x0ffff00ffff0000)){
                RECODE(C1TOSTACK1);    // PUSH1/GET_SP/PUSH1/ADD/STORE1
                next1_val = opcode4 >> 8;
                next1_addr = *(PC+3);
                SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
                *((uint8_t*) SPplus) = next1_val;
                PC+=6; STEPCOUNT_ACTION(4); NEXT;
            } else
            #endif
            #if (JUMP_PC_1_INSN > 0)
            if ((1UL*OPCODE_JUMP<<32 | OPCODE_ADD<<24 | OPCODE_GET_PC<<16) == (opcode8 & 0x0ffffff0000)){
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
        if ((1UL*OPCODE_JUMP<<40 | 1UL*OPCODE_ADD<<32 | OPCODE_GET_PC<<24) == (opcode8 & 0x0ffffff000000)){
            RECODE(JUMP_PC_2);    // PUSH2/GET_PC/ADD/JUMP
            next2 = opcode4 >> 8;
            PC += (WORD_T)next2 + 3;
            STEPCOUNT_ACTION(3); NEXT;
        } else
        #endif
        #if (C2TOSTACK8_INSN > 0)
        if ((1UL*OPCODE_STORE8<<56 | 1UL*OPCODE_ADD<<48 | 1UL*OPCODE_PUSH1<<32 | OPCODE_GET_SP<<24)
            == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK8);    // PUSH2/GET_SP/PUSH1/ADD/STORE8
            next2_val = opcode4 >> 8;
            next1_addr = *(PC+4);
            SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
            PC+=7;
            *((uint64_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (C2TOSTACK4_INSN > 0)
        if ((1UL*OPCODE_STORE4<<56 | 1UL*OPCODE_ADD<<48 | 1UL*OPCODE_PUSH1<<32 | OPCODE_GET_SP<<24)
            == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK4);    // PUSH2/GET_SP/PUSH1/ADD/STORE4
            next2_val = opcode4 >> 8;
            next1_addr = *(PC+4);
            SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
            PC+=7;
            *((uint32_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (C2TOSTACK2_INSN > 0)
        if ((1UL*OPCODE_STORE2<<56 | 1UL*OPCODE_ADD<<48 | 1UL*OPCODE_PUSH1<<32 | OPCODE_GET_SP<<24)
            == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK2);    // PUSH2/GET_SP/PUSH1/ADD/STORE2
            next2_val = opcode4 >> 8;
            next1_addr = *(PC+4);
            SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
            PC+=7;
            *((uint16_t*) SPplus) = next2_val;
            STEPCOUNT_ACTION(4); NEXT;
        } else
        #endif
        #if (C2TOSTACK1_INSN > 0)
        if ((1UL*OPCODE_STORE1<<56 | 1UL*OPCODE_ADD<<48 | 1UL*OPCODE_PUSH1<<32 | OPCODE_GET_SP<<24)
            == (opcode8 & 0x0ffff00ffff000000)) {
            RECODE(C2TOSTACK1);    // PUSH2/GET_SP/PUSH1/ADD/STORE1
            next2_val = opcode4 >> 8;
            next1_addr = *(PC+4);
            SPplus = (WORD_T)SP + (WORD_T)next1_addr - sizeof(WORD_T);
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
        if ((OPCODE_JUMP<<24 | OPCODE_ADD<<16 | OPCODE_GET_PC<<8) == (high4 & 0x0ffffff00)){
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
        #ifdef FPE_ENABLED
        push(v / u);
        #else
        push(u == 0 ? 0 : v / u);
        #endif
        NEXT;
    REM:
        u = pop();
        v = pop();
        #ifdef FPE_ENABLED
        push(v % u);
        #else
        push(u == 0 ? 0 : v % u);
        #endif
        NEXT;
    //-----------------
    // Logical works unsigned
    LT:
    #if OPTENABLED
        #ifdef PATTERN_LT
        #if (LT_JZF_INSN > 0)
        if (OPCODE_JZ_FWD<<8 == (opcode4 & 0x00ff00)){
            RECODE(LT_JZF);    // LT/JZF
            u = pop();
            v = pop();
            STEPCOUNT_ACTION(1);
            if (v < u) {
                PC+=2;
                NEXT;
            } else {
                next1 = opcode4 >> 16;
                PC += next1 + 2;
                NEXT;
            }
        } else
        #endif
        #if (LT_NOT_JZF_INSN > 0)
        if ((OPCODE_JZ_FWD<<16 | OPCODE_NOT<<8) == (opcode4 & 0x00ffff00)){
            RECODE(LT_NOT_JZF);    // LT/NOT/JZF
            u = pop();
            v = pop();
            STEPCOUNT_ACTION(2);
            if (v < u) {
                next1 = opcode4 >> 24;
                PC += next1 + 3;
                NEXT;
            } else {
                PC+=3;
                NEXT;
            }
        } else
        #endif
        #if (LT_JZB_INSN > 0)
        if (OPCODE_JZ_BACK<<8 == (opcode4 & 0x00ff00)){
            RECODE(LT_JZB);    // LT/JZB
            u = pop();
            v = pop();
            STEPCOUNT_ACTION(1);
            if (v < u) {
                PC+=2;
                NEXT;
            } else {
                next1 = opcode4 >> 16;
                PC -= next1 - 1;
                NEXT;
            }
        } else
        #endif
        #if (LT_NOT_JZB_INSN > 0)
        if ((OPCODE_JZ_BACK<<16 | OPCODE_NOT<<8) == (opcode4 & 0x00ffff00)){
            RECODE(LT_NOT_JZB);    // LT/NOT/JZB
            u = pop();
            v = pop();
            STEPCOUNT_ACTION(2);
            if (v < u) {
                next1 = opcode4 >> 24;
                PC -= next1 - 2;
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
        if ((OPCODE_LT<<24 | OPCODE_PUSH1<<8) == (opcode4 & 0x0ff00ff00)){
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
        push((u <= 63) ? (1UL << u) : 0);
        NEXT;
    //-----------------
    CHECK:
        x = pop();
        if (x > IVM_BINARY_VERSION) {
            longjmp(env, WRONG_BINARY_VERSION);
        }
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
        putc((int)(unsigned char)u, OUTPUT_PUTBYTE);
        NEXT;
    ADD_SAMPLE:
        a = pop();
        b = pop();
        //NO_IO: printing instruction
        fprintf(OUTPUT_PUTBYTE, "add_sample!! %lu %lu\n", a, b);
        NEXT;
    NEW_FRAME:
        a = pop();
        b = pop();
        u = pop();
        //NO_IO: printing instruction
        fprintf(OUTPUT_PUTBYTE, "new_frame!! %lu %lu %lu\n", a, b, u);
        NEXT;
    SET_PIXEL:
        a = pop();
        b = pop();
        u = pop();
        v = pop();
        r = pop();
        //NO_IO: printing instruction
        fprintf(OUTPUT_PUTBYTE, "set_pixel!!!!! %lu %lu %lu %lu %lu\n", a, b, u, v, r);
        NEXT;
    READ_PIXEL:
        a = pop();
        b = pop();
        //NO_IO: printing instruction
        fprintf(OUTPUT_PUTBYTE, "read_pixel!! %lu %lu\n", a, b);
        push(0);
        NEXT;
    READ_FRAME:
        a = pop();
        fprintf(OUTPUT_PUTBYTE, "read_frame! %lu\n", a);
        push(0);
        push(0);
        NEXT;
    READ_CHAR:
        TTY_NEW;
        x = getchar();
        TTY_DEF;
        if (x == EOF) {
            clearerr(stdin);
            x=4; // ascii 4 = ^D
        }
        push(x);
        NEXT;
    #else
    // from github.com/preservationvm/ivm-implementations/blob/master/OtherMachines/vm.c
    READ_FRAME:
        #ifdef PARALLEL_OUTPUT
        // wait all pending output
        while (nproc>0) {
           if (waitpid(-1, &status, 0))
                nproc--;
        }
        #endif
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
        #ifdef PARALLEL_OUTPUT
            if (nproc >= maxproc){
                waitpid(-1, &status, 0);
                nproc--;
            }
            pid_t pid = fork();
            if (pid == 0){
                child = 1;
                ioFlush();
                exit(0);
            } else if (pid > 0) {
                outputCounter++;
                currentText.used = 0;
                currentBytes.used = 0;
                currentSamples.used = 0;
                currentOutImage.used = 0;
                ioNewFrame(u, v, r);
                fflush(OUTPUT_PUTCHAR);
                nproc++;
                while ((nproc > 0) && (waitpid(-1, &status, WNOHANG) > 0)){nproc--;};
            } else {
                printf("** Error in fork\n");
                exit(EXIT_FAILURE);
            }
        #else
            ioFlush();
            ioNewFrame(u, v, r);
        #endif
        //printf("\r\f");
        NEXT;
    SET_PIXEL:
        b = pop(); a = pop(); r = pop();
        v = pop(); u = pop();
        ioSetPixel(u, v, r, a, b); // u -> x ; v -> y ; a -> r
        NEXT;
    ADD_SAMPLE:
        u = pop(); v = pop();
        ioAddSample(v, u); // u -> x ; v -> y
        NEXT;
    PUT_CHAR:
        u = pop();
        //putc((int)(unsigned char)u, OUTPUT_PUTCHAR); // Print char to stderr or stdout
        ioPutChar(u);
//fflush(NULL);
        NEXT;
    PUT_BYTE:
        u = pop();
        //putc((int)(unsigned char)u, OUTPUT_PUTBYTE);
        ioPutByte(u);
        NEXT;
    //PUT_CHAR: ioPutChar(pop()); NEXT;
    //PUT_BYTE: ioPutByte(pop()); NEXT;
    READ_CHAR:
        TTY_NEW;
        u = ioReadChar();
//fflush(NULL);
        TTY_DEF;
        if (feof(stdin)) {
            clearerr(stdin);
        }
        push(u);
        NEXT;
    #endif

    //-----------------
    BREAK:
        #if (VERBOSE >= 2)
        getchar();
        #endif
        #if (VERBOSE<3)
        STEPCOUNT_ACTION(-1);
        #endif
        NEXT;
    TRACE:
        next1 = *((uint8_t*)PC);
        PC+=1;
        #if (VERBOSE >= 2)
        trace = next1;
        #endif
        #if (VERBOSE<3)
        STEPCOUNT_ACTION(-1);
        #endif
        NEXT;
    PROBE:
        probe = *((uint8_t*)PC);
        PC+=1;
        #if (VERBOSE<3)
        STEPCOUNT_ACTION(-1);
        #endif
        NEXT;
    PROBE_READ:
        read_probe = pop();
        a = pop();
        #ifdef STEPCOUNT
        *(uint64_t*)a = samples[read_probe];
        #endif
        #if (VERBOSE<3)
        STEPCOUNT_ACTION(-1);
        #endif
        NEXT;
    //-----------------

    HALT:

    signal(SIGINT,SIG_DFL);
    signal(SIGSEGV,SIG_DFL);
    signal(SIGFPE,SIG_DFL);

    // At exit, reset std stream orientation
    reset_std_streams();
    TTY_DEF;

    #ifdef PARALLEL_OUTPUT
    if (child){
        exit(0);
    } else {
       while (nproc>0) {
           if (waitpid(-1, &status, 0))
                nproc--;
       }
    }
    #endif

    #ifdef WITH_IO
    ioFlush();
    #endif
    fprintf(OUTPUT_MSG, "\n");

    #define HUMANSIZE(x) ((double)(((x)>1e12)?((x)/1.0e12):((x)>1e9)?((x)/1.0e9):((x)>1.0e6)?((x)/1.0e6):((x)>1e3)?((x)/1.0e3):(x)))
    #define HUMANPREFIX(x)  (((x)>1e12)?"T":((x)>1e9)?"G":((x)>1e6)?"M":((x)>1e3)?"K":"")

    #ifdef STEPCOUNT
        long binsize = execEnd-execStart+1;
        long steps = 0;

        for (int i=0; i < 256; i++) steps += samples[i];

        if (steps != samples[0]) {
            for (int i=0; i < 256; i++) {
                if (samples[i] > 0) {
                    printf("Probe %3d: %10ld\n", i, samples[i]);
                }
            }
        }

        #if (defined(HISTOGRAM) && !defined(NOOPT))
        fprintf(OUTPUT_MSG, "Binary file size: %lu bytes (%.1f %sB)\n",
                             binsize, HUMANSIZE(binsize), HUMANPREFIX(binsize));
        fprintf(OUTPUT_MSG, "Executed %lu instructions; %lu fetches (%4.2lf insn per fetch)\n\n",
                            steps, fetchs, (double)steps/fetchs);
        #else
        fprintf(OUTPUT_MSG, "Binary file size: %lu bytes (%.1f %sB)\n",
                             binsize, HUMANSIZE(binsize), HUMANPREFIX(binsize));
        fprintf(OUTPUT_MSG, "Executed %lu instructions (%.2f %si)\n\n",
                             steps, HUMANSIZE(steps), HUMANPREFIX(steps));
        #endif


    #endif

    #ifdef HISTOGRAM
    for (int i=0; i < 256; i++) {
        if (histogram[i] > 0) {
            printf("%15ld\t%-10s\t%6.3lf%%\t%15ld\t%20.2lf\n",
                    histogram[i], insn_attributes[i].name,
                    (double)histogram[i]/fetchs*100, histo2[i],
                    histo2[i]?(double)histogram[i]/histo2[i]:histogram[i]);
        }
    }
    #endif

    int ret_val;
    if (error == SIGSEGV) {
        fprintf(OUTPUT_MSG, "error: segmentation fault\n\n");
    } else  if (error == SIGFPE) {
        fprintf(OUTPUT_MSG, "error: division by zero\n\n");
    } else if (error == SIGINT) {
        fprintf(OUTPUT_MSG, "Program terminated by user request ^C\n\n");
    } else if (error == WRONG_BINARY_VERSION) {
        fprintf(OUTPUT_MSG, "Incompatible binary version: %ld\n\n", x);
    }

    if ( addr2idx(SP) < execStart || addr2idx(SP) >= MemBytes) {
        fprintf(OUTPUT_MSG, "End stack:\nSP=%p out of range: 0x%lx [0x%lx 0x%lx]\n", SP, addr2idx(SP),
				execStart, MemBytes);
        ret_val = EXIT_FAILURE;
    } else {
        unsigned long nstack = (idx2addr(MemBytes - BYTESPERWORD) - SP)/sizeof(WORD_T);
        // If IVM_EMU_MAX_DUMPED_STACK is defined use as the max number of stack position shown
        // otherwise, show all (nstack)
        unsigned long ntop = 31; // By default show only the 32 stack top positions

        char *ntop_str =  getenv("IVM_EMU_MAX_DUMPED_STACK");
        if (ntop_str) {
            ntop = atol(ntop_str);
        } else {
            char *full_stk_str =  getenv("IVM_EMU_DUMP_FULL_STACK");
            if (full_stk_str) {
                ntop = nstack;
            }
        }

        ret_val = *((uint64_t*)SP);

        // Print the stack in the same format than the vm implementation
        print_stack(FORMAT_STACK_IVM, ntop);

        fprintf(OUTPUT_MSG, "Shown top %lu out of %lu stack positions\n", MIN(ntop+1, nstack), nstack);
        fprintf(OUTPUT_MSG, " (export IVM_EMU_MAX_DUMPED_STACK=N to show N+1 stack positions only)\n");
        fprintf(OUTPUT_MSG, " (to show all stack positions, unset IVM_EMU_MAX_DUMPED_STACK and export IVM_EMU_DUMP_FULL_STACK=1)\n");

    }
    fflush(NULL);

    if (error) {
        fprintf(OUTPUT_MSG, "Last known instruction\n");
        if ( addr2idx(PC-1) < execStart || addr2idx(PC-1) > execEnd){
            printf("PC=%p out of range\n", PC-1);
        } else {
            print_insn(PC-1);
        }

        //#if (VERBOSE>=3)
        // In case of error, print the nearest labels if available
        symrec *symL, *symU;
        find_nearest_label(Ts, addr2idx(PC-1), &symL, &symU);
        if (symL) fprintf(OUTPUT_MSG, "   Nearest lower label: %s\n", symL->label);
        if (symU) fprintf(OUTPUT_MSG, "   Nearest upper label: %s\n", symU->label);
        //#endif
    }
    fflush(NULL);

    //#if (VERBOSE >= 3)
    destroy_symtable(Ts);
    //#endif

    if (error) {
		if (error == SIGSEGV) {
		    // real seg fault again;
			// The script running the tests want the same behaviour in ivm64-gcc as in gcc
			// 	so, programs the seg.fault with gcc should seg.fault with ivm_emu
			char *p = 0;
			*p = 0;
		} else if (error == WRONG_BINARY_VERSION){
            ret_val = WRONG_BINARY_VERSION_RET_VALUE;
        } else {
		    ret_val = error | 0x80;
		}
    }

    return ret_val;
}
