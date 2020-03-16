# Pool-Memory-Allocator

* Single Templated Header File.
* Support Multithread Environment.
* Can Allocated Contiguous Blocks of memory and access them via [] operator as a normal array of objects ( _supportArrayAllocation = true )
* Beats Standards new/delete C++ operators in allocation/free time in single allocation mode ( _supportArrayAllocation = false ).
* Gives a fairly good timing in allocation of Contiguous Blocks.
* Needs improvement in de-allocation time while in *_supportArrayAllocation* mode.
