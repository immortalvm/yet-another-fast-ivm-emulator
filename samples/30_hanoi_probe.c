/* example from http://barnyard.syr.edu/quickies/hanoi.c */

/* hanoi.c: solves the tower of hanoi problem. (Programming exercise.) */
/* By Terry R. McConnell (12/2/97) */
/* Compile: cc -o hanoi hanoi.c */

/* This program does no error checking. But then, if it's right, 
   it's right ... right ? */


/* The original towers of hanoi problem seems to have been originally posed
   by one M. Claus in 1883. There is a popular legend that goes along with
   it that has been often repeated and paraphrased. It goes something like this:
   In the great temple at Benares there are 3 golden spikes. On one of them,
   God placed 64 disks increasing in size from bottom to top, at the beginning
   of time. Since then, and to this day, the priest on duty constantly transfers
   disks, one at a time, in such a way that no larger disk is ever put on top
   of a smaller one. When the disks have been transferred entirely to another
   spike the Universe will come to an end in a large thunderclap.

   This paraphrases the original legend due to DeParville, La Nature, Paris 1884,
   Part I, 285-286. For this and further information see: Mathematical 
   Recreations & Essays, W.W. Rouse Ball, MacMillan, NewYork, 11th Ed. 1967,
   303-305.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

/* This is the number of "disks" on tower A initially. Taken to be 64 in the
 * legend. The number of moves required, in general, is 2^N - 1. For N = 64,
 * this is 18,446,744,073,709,551,615 */
#define N 8

/* These are the three towers. For example if the state of A is 0,1,3,4, that
 * means that there are three discs on A of sizes 1, 3, and 4. (Think of right
 * as being the "down" direction.) */
int A[N], B[N], C[N]; 

void Hanoi(int,int*,int*,int*);

/* Print the current configuration of A, B, and C to the screen */
void PrintAll()
{
   int i;

   printf("A: ");
   for(i=0;i<N;i++)printf(" %d ",A[i]);
   printf("\n");

   printf("B: ");
   for(i=0;i<N;i++)printf(" %d ",B[i]);
   printf("\n");

   printf("C: ");
   for(i=0;i<N;i++)printf(" %d ",C[i]);
   printf("\n");
   printf("------------------------------------------\n");
   return;
}

/* Move the leftmost nonzero element of source to dest, leave behind 0. */
/* Returns the value moved (not used.) */
int Move(int *source, int *dest)
{
   int i = 0, j = 0;

   while (i<N && (source[i])==0) i++;
   while (j<N && (dest[j])==0) j++;

   dest[j-1] = source[i];
   source[i] = 0;
   PrintAll();       /* Print configuration after each move. */
   return dest[j-1];
}


/* Moves first n nonzero numbers from source to dest using the rules of Hanoi.
   Calls itself recursively.
   */
void Hanoi(int n,int *source, int *dest, int *spare)
{
   int i;
   if(n==1){
      Move(source,dest);
      return;
   }

   Hanoi(n-1,source,spare,dest);
   Move(source,dest);
   Hanoi(n-1,spare,dest,source);	
   return;
}

#include "probe.h"
//#define WITH_IVM64_PROBES
// compile with:  ivm64-gcc 30_hanoi_probe.c -DWITH_IVM64_PROBES -o 30_hanoi_probe
// assemble with: ivm as 30_hanoi_probe
// execute with:  ivm_probe 30_hanoi_probe.b
long probes[4];

int main()
{
   // default probe is 0 (start code)
   ivm64_set_probe(1); // after this, probe 0 is used. Instructions of main() function
   int i;

   /* initialize the towers */
   ivm64_set_probe(2); // after this, probe 2 is used. Probe 1 is not sampling
   for(i=0;i<N;i++)A[i]=i+1;
   for(i=0;i<N;i++)B[i]=0;
   for(i=0;i<N;i++)C[i]=0;
   ivm64_set_probe(1); // after this, probe 1 is used again

   printf("Solution of Tower of Hanoi Problem with %d Disks\n\n",N);

   /* Print the starting state */
   printf("Starting state:\n");
   PrintAll();
   printf("\n\nSubsequent states:\n\n");

   /* Do it! Use A = Source, B = Destination, C = Spare */
   ivm64_set_probe(3); // after this, probe 3 is used
   Hanoi(N,A,B,C);

   ivm64_set_probe(1); // after this, probe 1 is used again
   for(i=0;i<4;i++)ivm64_read_probe(i,probes[i]);

   for(i=0;i<4;i++)printf("Probe %d: %9ld\n",i,probes[i]);

   ivm64_set_probe(0); // after this (exit code), probe 0 is used again
   // instruction of main() is the sum of probes 1, 2 and 3
   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
