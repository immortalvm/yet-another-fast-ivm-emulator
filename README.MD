# Yet another (fast) IVM emulator

This ivm64 emulator takes an ivm64 binary and emulates its execution. It benefits from two kind of optimizations:

  * Optimized execution of common instruction patterns generated by the C/C++ compiler
  * Parallel execution of output instructions based on multiple processes

## Requirements 

For versions with I/O, library png is needed. 

In a deb based linux distribution, it can be installed as:

```bash
sudo apt update
sudo apt install libpng-dev
```

## How to compile?

In order to have the emulator ```ivm64-emu```, build the project by calling make:

<font size="0">

```bash
  $ make # generates ivm64-emu and some other executables
  gcc ivm_emu.c -o ivm64-emu               -DSTEPCOUNT -DWITH_IO -lpng
  gcc ivm_emu.c -o ivm64-emu-trace_compact -DSTEPCOUNT -DWITH_IO -DNOOPT -DVERBOSE=3 -lpng

  gcc ivm_emu.c -o ivm64-emu-no-io    -DSTEPCOUNT
  gcc ivm_emu.c -o ivm64-emu-parallel -DSTEPCOUNT -DWITH_IO -DPARALLEL_OUTPUT -lpng

  gcc ivm_emu.c -o ivm64-emu-histo    -DSTEPCOUNT -DWITH_IO -DNOOPT -DVERBOSE=1 -DHISTOGRAM -lpng
  gcc ivm_emu.c -o ivm64-emu-trace2   -DSTEPCOUNT -DWITH_IO -DNOOPT -DVERBOSE=2 -lpng
  gcc ivm_emu.c -o ivm64-emu-trace4   -DSTEPCOUNT -DWITH_IO -DNOOPT -DVERBOSE=4 -lpng
```

</font>

Also you can build your customized emulator combining different options. The following ones are for debugging and they can make the execution slower:

<font size="0">

```bash
  gcc  -DFPE_ENABLED ivm_emu.c # Generate exception when dividing by zero
  gcc  -DNOOPT       ivm_emu.c # Disable optimizations
  gcc  -DSTEPCOUNT   ivm_emu.c # Enable instruction count, show probes if used
  gcc  -DVERBOSE=1   ivm_emu.c # Some extra info
  gcc  -DVERBOSE=2   ivm_emu.c # Trace mode (show instruction sequence)
  gcc  -DVERBOSE=3   ivm_emu.c # Trace mode w/ compact output (similar to original ivm app)
  gcc  -DVERBOSE=4   ivm_emu.c # Detailed trace mode (instr. and stack content sequence)
  gcc  -DHISTOGRAM   ivm_emu.c # Enable insn. pattern histogram
```

</font>

The number of processes for the version with parallel output is 8 by default. If compiled with -DNUM_THREADS=N1, N1 is used instead of the default value. If set the environment variable NUM_THREADS=N2, N2 is used instead of N1 or default. In any case, the parallel version uses at least 2 threads, in general: 1 thread for emulation and (N-1) thread for io.
## How to execute?

Options keep compatibility with the original ivm implementation.

```ivm64-emu [-m <size in bytes>] [-a <arg file> [-a <env file>]] [-i <input dir>] [-o <output dir>] <ivm_binary_file> ```

This is the meaning of the options:

  * ```-m <size in bytes>```: sets the size in bytes of the emulated virtual machine memory
  * ```-a <arg file>```: specifies an argument file (in case of a ivm code generated by the ```ivm64-gcc``` compiler, the c run time crt0 parses this argument file as common linux process arguments found in file ```/proc/<pid>/comdline```; additionally a second ```-a``` option allows specifying an environment file that is processed by crt0 as the same format of linux ```/proc/<pid>/environment```)
  * ```-i <input dir>```: in this directory, input instructions will find the data
  * ```-o <output dir>```: in this directory, output instructions will write data



## Debugging

In addition to ```ivm64-emu```, version 2.0 can be compiled to generate executables that are useful for
debugging. 

Examples of this capabilities are: ```ivm64-emu-histo```, and  ```ivm64-emu-trace_compact```.
These last two allow the execution showing the histogram of instructions or a traced execution with a
compact output, respectively.

<!---
## Profiling

To allow dynamically activation/deactivation of program tracing, and to extract information of
executed instructions in a code section, the ```Yet another IVM emulator``` admits three unnused
opcodes: 0xf0, 0xf1, 0xf2 and 0xf3.

In directory ```samples```, a C code using this facility can be found: ```30_hanoi_probe.c```.
This sample code includes the header ```probe.h``` which contains the macros to easily insert the
appropiate opcodes into the C code (as if they were function calls).

After this, compile the program and assemble it:

```ivm64-gcc 30_hanoi_probe.c -o 30_hanoi_probe -DWITH_IVM64_PROBES```

```ivm64-as 30_hanoi_probe```

At this point, the binary contains non-standard opcodes, so it can not be executed using this emualtor:

Execute the binary using Yet another IVM emulator:
```ivm64-emu-probe 30_hanoi_probe.b```

The program makes it normal execution, printing all its output and at the end, the emulator
print the gathered information, for example:

<font size="0">

```
Binary file size: 172315 bytes (172.3 KB)
Executed 38070971 instructions

Probe   0:      13732
Probe   1:     180166
Probe   2:       2936
Probe   3:   37874142
End stack:
0x..000000 0
```

</font>

Probes can also be read from the program if requiered using ```ivm64_probe_read()```.

To obtain a binary without the non-standard opcodes, just compile the program without WITH_IVM64_PROBES:
```ivm64-gcc 30_hanoi_probe.c -o 30_hanoi_probe```

This way, all sentences like  ```ivm64_set_probe(1)```,```ivm64_trace_off```,```ivm64_trace_hard```,
etc, are ignored.

--->

## Execution times

These experiments were conducted on an Intel(R) Core(TM) i7-3770 CPU @ 3.40GHz, where the emulator was compiled with gcc 7.4.0.

Code static_unboxing was compiled with gcc 10.2.0-ivm64-2.0. This code has no I/O instructions but ```put_char```.

Code ex4_short_video.s is one of the example codes provided with the ivm application.

Binaries generated with ivm v0.37.

| benchmark                      | no optimization | optimized |  optimized +  parallel io |
|--------------------------------|-----------------|-----------|---------------------------|
| static_unboxing                |1m30s            | 35s       |               |
| ex4_short_video.s (512 frames) |7s               | 3.6s      |  1.8s         |

