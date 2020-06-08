/*
To test ivm image input/output
    ivm64-gcc -O2 ex_ivm_io.c -o ex_ivm_io.s
    ivm as ex_ivm_io.s

    # Choose favourite emulator
    IVM_EMU="ivm run"
    IVM_EMU="ivm_emu_fast_io_parallel"

    $IVM_EMU -i /path/to/in -o /path/to/out ex_ivm_io.b 

    # Interesting tests
    # Same input and output dir 
        $IVM_EMU -i /path/to/in -o /path/to/in ex_ivm_io.b 
    # No output dir
        $IVM_EMU -i /path/to/in ex_ivm_io.b 

*/

#include <stdio.h>
#include <stdlib.h>

void ivm64_new_frame(long width, long height, long rate)
{
#ifdef __ivm64__
  asm volatile("new_frame* [(load8 %[w]) (load8 %[h]) (load8 %[r])]"
               : : [w] "m" (width), [h] "m" (height), [r] "m" (rate));
#else
  printf ("new_frame* [%ld %ld %ld]\n", width, height, rate);
#endif
}

void ivm64_set_pixel(long x, long y, long r, long g, long b) 
{
#ifdef __ivm64__
  asm volatile("set_pixel* [(load8 %[x]) (load8 %[y]) (load8 %[r]) (load8 %[g]) (load8 %[b])]"
               : : [x] "m" (x), [y] "m" (y), [r] "m" (r), [g] "m" (g), [b] "m" (b));
#else
  printf ("set_pixel* [%ld %ld %ld %ld %ld])\n", x, y, r, g, b);
#endif
}

void ivm64_read_frame(long* widthp, long* heightp, long frameindex) 
{
  static long width = 0, height = 0;
  /* Could we avoid this intermediate storage,
     and instead write directly to the args in the asm below? */

#ifdef __ivm64__
  /* Pushes 16 bytes on the stack (first width, then height): */
  asm volatile("read_frame! (load8 %[n])" : : [n] "m" (frameindex));

  /* Pops 8 bytes form the stack: */
  asm volatile("store8! %[h]" : [h] "=m" (height) : );
  /* Idem: */
  asm volatile("store8! %[w]" : [w] "=m" (width) : );
#else
  printf ("read_frame index=%lu\n",frameindex);
#endif

  *widthp = width;
  *heightp = height;
}

long ivm64_read_pixel(long x, long y)
{
  long v = 0;

#ifdef __ivm64__
  asm volatile("read_pixel* [(load8 %[x]) (load8 %[y])]"
               : : [x] "m" (x), [y] "m" (y) ); /* Pushes 8 bytes on the stack */
  asm volatile("store8! (+ 8 %[v])"
               : [v] "=m" (v) : ); /* Pops 8 bytes */
#else
  printf ("read_pixel* [%ld %ld])\n", x, y);
  printf ("<result value on stack: %ld>\n", v);
#endif

  return v;
}

#define NUMFRAMES 100 
#define BITRATE 10

#define MAXPIXELS 20 
#define SAMPLETIME 10 

#define MAX(a,b) ((a>b)?a:b)

int W[5]={128, 256, 512, 1024, 2048};
int H[5]={128, 256, 512, 1024, 2048};

int main(){

    int k=0; 
    long int width, height;

    srand((long int)&main);

    for (k=0; k < NUMFRAMES; k++){
        width  = W[k % 5];
        height = H[k % 5];
        ivm64_new_frame(width, height, BITRATE);

        for (int t=0; t < rand()% MAXPIXELS; t++)
            ivm64_set_pixel(rand() % width, rand() % height, 255, 127, 1); 

        if (!(rand() % SAMPLETIME)){
            width = 0;
            height = 0;
            int f = MAX(1, k-1);
            ivm64_read_frame(&width, &height, f); 
            printf("k=%d -> read frame %d, width=%ld, height=%ld\n", k, f, width, height);
        }
    }

    return k-1;
}
