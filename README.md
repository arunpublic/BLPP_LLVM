# BLPP_LLVM
An Implementation of BLPP in LLVM

Usage:

Instrumentation:

opt -load LLVMPathProfiler.so -ppinstrument loop.bc -o loop.ins.bc

Create Instrumented Executable: Link with PPInfoSerializer library to generate instrumented executable:

g++ loop.ins.o libPPInfoSerializer.a -o loop.ins

Generate Path Profile: Run the instrumented executable to generate path profile (prof.res)

Analyse Path Profile Data: The BLPPDB class allows us to query for the set of hot paths between a pair of nodes (as recorded by the BLPP algorithm). Currently, BLPPDump.so is the wrapper around it, and can be used like this:

opt -load BLPPDump.so -blppdump -blppdata prof.res loop.bc

The output for the test program and a sample profile looks like:

Path ID: 0;0->1->2->3->EOP:1

Path ID: 2;1->2->3->EOP:10

Path ID: 3;1->4->EOP:1

In this case, during profile run, the loop in the test program is executed 11 times (Path 0 - once, Path 2 - 10 times).


References:

* Efficient Path Profiling (BLPP) paper

* Implementation in machsuif

Tested only for a simple factorial program

TODO - IMMEDIATE REQUIREMENTS:

1) Generate unique path-ids for the same sequence of BB's (Current implementation uses llvm's iterators to order the successors of a BLPP graph node, while we want a deterministic order across different runs)

2) Generate unique ids to functions (Current implementation uses llvm's iterators to examine functions and assign id's while we want a deterministic order)

3) Test and validate
