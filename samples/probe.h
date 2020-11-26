#ifndef _PROBE_H_
#define _PROBE_H_

// include this header, define WITH_IVM64_PROBES
#ifdef WITH_IVM64_PROBES

// wait for key pressed if VERBOSE>=2 else do nothing
#define ivm64_break_point() __asm__ volatile ("data1 [ 0xf0]");

// activate/deactivate trace if VERBOSE>=2 else do nothing
// switch trace off
#define ivm64_trace_off()   __asm__ volatile ("data1 [ 0xf1 0 ]")
// trace instructions only
#define ivm64_trace_soft()  __asm__ volatile ("data1 [ 0xf1 1 ]")
// trace instructions and stack
#define ivm64_trace_hard()  __asm__ volatile ("data1 [ 0xf1 2 ]")

// if VERBOSE>=2
//   switch to probe number n (of 256) and [re]start count instructions
// else do nothing
#define ivm64_set_probe(n)  __asm__ volatile ("data1 [ 0xf2 %0 ]"::"i"(n))


#else
// Do not produce opcodes 0xf0, 0xf1, 0xf2
#define ivm64_break_point()
#define ivm64_trace_off()
#define ivm64_trace_soft()
#define ivm64_trace_hard()
#define ivm64_set_probe(n)

#endif



#endif
