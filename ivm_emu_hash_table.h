/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Original emulator:
 Authors:
  Eladio Gutierrez Carrasco
  Sergio Romero Montiel
  Oscar Plata Gonzalez

 Date: Jan 2023
*/

#ifndef __IVM_EMU_HASH_TABLE_H
#define __IVM_EMU_HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>

// Hashed symbol table based on the simple list symbol table
// from: Compiler Construction using Flex and Bison, Anthony A. Aaby, 2003
// https://dlsiis.fi.upm.es/traductores/Software/Flex-Bison.pdf
struct symrec_s
{
    unsigned long pc;      /* position associated to the label */
    char *label;           /* name of label (symbol)*/
    struct symrec_s *next; /* link field */
};
typedef struct symrec_s symrec;

typedef struct sym_table_s {
    /* Symbol hash table, each entry is a pointer to
       a symbol linked list of symrec */
    /* sym_table -> sym_table[0] -> sym_rec -> sym_rec ....
                    sym_table[1] -> sym_rec -> sym_rec ....
                    ... */
    symrec** sym_table;
    unsigned long int sym_table_size;
} sym_table_t;

static sym_table_t* init_symtable(unsigned long size){

   sym_table_t *T = (sym_table_t*)malloc(sizeof(sym_table_t));

   T->sym_table = (symrec**)malloc(size*sizeof(symrec*));
   for (long i=0; i<size; i++){
        T->sym_table[i] = (symrec*)0;
   }
   T->sym_table_size = size;
   return T;
   // fprintf(stderr, "Symbol table initialized with %ld entries\n", sym_table_size); //debug
}

static void destroy_symtable(sym_table_t *T){
    /* Deallocate all records in the list for each
       not empty table entry */
    /*TODO: implement this*/
}

/*getsym which returns a pointer to the symbol table entry corresponding
  to an identifier, in this case the identifier is the pc*/
static symrec* getsym(unsigned long pc, symrec* sym_table)
{
    symrec *ptr;
    for (ptr = sym_table; ptr != (symrec *)0; ptr = (symrec *)ptr->next)
        if (ptr->pc == pc)
            return ptr;
    return 0;
}

/*Two operations: putsym to put an identifier into the table*/
static symrec* putsym(unsigned long pc, char *label, symrec** sym_table, int update)
{
    symrec *ptr = getsym(pc, *sym_table);
    if (!ptr) {
        /* New node */
        ptr = (symrec *) malloc (sizeof(symrec));
        ptr->next = (symrec *)(*sym_table);
        *sym_table = ptr;
        ptr->pc = pc;
        ptr->label = strdup(label);
    } else if (update) {
        /* Update an existing symbol if update is set */
        ptr->pc = pc;
        if (ptr->label) free(ptr->label);
        ptr->label = strdup(label);
    }
    return ptr;
}

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
static unsigned long hashfun(uint64_t x, unsigned long maxval){
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9UL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebUL;
    x = x ^ (x >> 31);
    return x % maxval;
}

/*
static unsigned long hashfun(unsigned long N, unsigned long maxval){
    unsigned long x = N;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x % maxval;
}
*/

/* Hashed versions of putsym, getsym */
static symrec* putsym_hash(sym_table_t *T, unsigned long pc, char *label, int update){
    if (!T) return NULL;
    long hash = hashfun(pc, T->sym_table_size);
    return putsym(pc, label, &T->sym_table[hash], update);
}

static symrec* getsym_hash(sym_table_t *T, unsigned long pc){
    if (!T) return NULL;
    long hash = hashfun(pc, T->sym_table_size);
    symrec* r = getsym(pc, T->sym_table[hash]);
    return r;
}

static long print_symtable(sym_table_t *T)
{
    symrec *p;
    long nsym = 0;
    for (long i=0; i < T->sym_table_size; i++){
        for (p = T->sym_table[i]; p != (symrec *)0; p = (symrec *)p->next){
            fprintf(stderr, "pc=%ld ---> '%s'\n", p->pc, p->label);
            nsym++;
        }
    }
    return nsym;
}

/* Find the symbols nearest (above and below) to a given logic PC position
   The argument pc, is the byte-index in memory (starting in 0) not the
   physical host position (use add2idx to convert from the two spaces).
*/
static void find_nearest_label(sym_table_t *T, unsigned long pc, symrec **sL, symrec **sU)
{
    symrec *symL = NULL, *symU = NULL;
    symrec *p;


    for (long i=0; i < T->sym_table_size; i++){
        for (p = T->sym_table[i]; p != (symrec *)0; p = (symrec *)p->next){
            //fprintf(stderr, "pc=%ld ---> '%s'\n", p->pc, p->label);
            if ((symL && p->pc <= pc &&  p->pc > symL->pc) || !symL) {
                symL = p;
            }
            if ( p->pc > pc && !symU) {symU = p;}
            else if ((symU && p->pc > pc &&  p->pc < symU->pc)) {
                symU = p;
            }
        }
    }
    *sL = symL;
    *sU = symU;
}

#endif //__IVM_EMU_HASH_TABLE_H
