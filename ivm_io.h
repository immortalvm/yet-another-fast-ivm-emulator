/*
 Preservation Virtual Machine Project

 Yet another ivm emulator

 Original Author:
    Ivar Rummelhoff
    from github.com/preservationvm/ivm-implementations/blob/master/OtherMachines/vm.c

 Reentrant version:
    Sergio Romero Montiel
    Eladio Gutierrez Carrasco
    Oscar Plata Gonzalez

 Date: May 2020

 IVM input/output instruction header file
*/

#ifndef __IVM_IO_H__
#define __IVM_IO_H__

typedef struct OutputElement* OutputElement_t;
extern OutputElement_t current;

#ifndef PARALLEL_OUTPUT
    // interface function for serial output
    #define ioFlush()   ioFlush_r(current)
#else
    #include "io_handler.h" // ioEnqueue(), ioInitParallel()
    #define ioFlush()   ioEnqueue(current)
#endif

// init functions
extern void ioInitIn();
extern void ioInitOut();
extern void ioFlush_r(OutputElement_t ioElem);

extern void ioNewFrame(uint16_t width, uint16_t height, uint32_t sampleRate);
extern void ioPutChar(uint32_t c);
extern void ioPutByte(uint8_t x);
extern void ioAddSample(uint16_t left, uint16_t right);
extern void ioSetPixel(uint16_t x, uint16_t y, uint64_t r, uint64_t g, uint64_t b);

extern void ioReadFrame(uint64_t i, uint64_t* width, uint64_t* height);
extern uint8_t ioReadPixel(uint16_t x, uint16_t y);

#endif
